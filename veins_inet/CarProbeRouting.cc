#include "CarProbeRouting.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/base/utils/Coord.h"

Define_Module(CarProbeRouting);

void CarProbeRouting::initialize(int stage) {
    BaseWaveApplLayer::initialize(stage);

    if (stage == 0) {
        numNodes = par("numNodes");
        startProbeNodeId = par("startProbeNodeId");
        probeInterval = par("probeInterval");

        myId = getParentModule()->getId();

        mobility = TraCIMobilityAccess().get(getParentModule());

        initializeRoutingTable();

        if (myId == startProbeNodeId) {
            startProbeRouting();
        }

    } else if (stage == BaseWaveApplLayer::LOWER_LAYER_INITIALIZATION_STAGE) {

    }
}

void CarProbeRouting::initializeRoutingTable() {
    routingTable.clear();
    for (int j = 0; j < numNodes; ++j) {
        if (j != myId) {
            routingTable[j] = std::vector<int>();
        }
    }
}

void CarProbeRouting::startProbeRouting() {
    sendProbe();
}

std::vector<bool> CarProbeRouting::createInitialBitmap() {
    std::vector<bool> bitmap(numNodes, false);
    if (myId >= 0 && myId < numNodes) {
        bitmap[myId] = true;
    } else {
         EV_ERROR << "Node " << myId << ": Invalid node ID for bitmap creation!" << endl;
    }
    return bitmap;
}

void CarProbeRouting::sendProbe(int destination) {
    ProbeMessage *probeMsg = new ProbeMessage();
    probeMsg->setSourceNodeId(myId);
    probeMsg->setBitmap(createInitialBitmap());

    probeMsg->setWsmVersion(1);
    probeMsg->setSecurityType(static_cast<int>(veins::SecurityType::UNSECURED));
    probeMsg->setChannelNumber(static_cast<int>(TraCIConstants::Channel::CCH));
    probeMsg->setPsid(0);
    probeMsg->setPriority(3);
    probeMsg->setSenderAddress(myId);
    probeMsg->setRecipientAddress((destination == -1) ? LAddress::L2BROADCAST() : destination);
    probeMsg->setSenderPos(myPosition);
    probeMsg->setTimestamp(simTime());
    probeMsg->setSerial(0);

    populateWSM(probeMsg);
    sendDown(probeMsg);
}

void CarProbeRouting::handleLowerMsg(cMessage* msg) {
    ProbeMessage *probeMsg = dynamic_cast<ProbeMessage*>(msg);
    if (probeMsg) {
        if (probeMsg->getSenderAddress() != myId) {
            processProbe(probeMsg);
        }
    } else {
        EV << "Node " << myId << ": Received non-probe message from lower layer. Type: " << msg->getClassName() << endl;
    }
    delete msg;
}

void CarProbeRouting::processProbe(ProbeMessage* probeMsg) {
    int senderId = probeMsg->getSenderAddress();
    const std::vector<bool>& receivedBitmap = probeMsg->getBitmap();
    if (receivedBitmap.size() != numNodes) {
        EV_ERROR << "Node " << myId << ": Received bitmap size mismatch! Expected " << numNodes << ", got " << receivedBitmap.size() << ". Ignoring probe.";
        return;
    }

    bool potentiallyForward = false;

    for (int targetNodeId = 0; targetNodeId < numNodes; ++targetNodeId) {
        if (targetNodeId != myId && receivedBitmap[targetNodeId]) {
            if (updateRoutingTable(targetNodeId, senderId, receivedBitmap)) {
                 potentiallyForward = true;
            }
        }
    }

    if (potentiallyForward) {
        ProbeMessage *forwardProbeMsg = new ProbeMessage();
        forwardProbeMsg->setSourceNodeId(myId);

        std::vector<bool> newBitmap = mergeBitmaps(createInitialBitmap(), receivedBitmap);
        forwardProbeMsg->setBitmap(newBitmap);

        forwardProbeMsg->setWsmVersion(1);
        forwardProbeMsg->setSecurityType(static_cast<int>(veins::SecurityType::UNSECURED));
        forwardProbeMsg->setChannelNumber(static_cast<int>(TraCIConstants::Channel::CCH));
        forwardProbeMsg->setPsid(0);
        forwardProbeMsg->setPriority(3);
        forwardProbeMsg->setSenderAddress(myId);
        forwardProbeMsg->setRecipientAddress(LAddress::L2BROADCAST());
        forwardProbeMsg->setSenderPos(myPosition);
        forwardProbeMsg->setTimestamp(simTime());
        forwardProbeMsg->setSerial(probeMsg->getSerial() + 1);

        populateWSM(forwardProbeMsg);
        sendDown(forwardProbeMsg);

    } else {
        EV << "Node " << myId << ": Probe from " << senderId << " contained no new information. Probe destroyed." << endl;
    }
}

bool CarProbeRouting::updateRoutingTable(int targetNodeId, int neighborId, const std::vector<bool>& receivedBitmap) {
    bool found = false;
    if (routingTable.count(targetNodeId)) {
         for (int existingNeighbor : routingTable[targetNodeId]) {
             if (existingNeighbor == neighborId) {
                 found = true;
                 break;
             }
         }
    } else {
        routingTable[targetNodeId] = std::vector<int>();
    }

    if (!found) {
        routingTable[targetNodeId].push_back(neighborId);
        return true;
    }

    return false;
}

std::vector<bool> CarProbeRouting::mergeBitmaps(const std::vector<bool>& currentMap, const std::vector<bool>& receivedMap) {
    std::vector<bool> mergedMap(numNodes, false);
    for (int i = 0; i < numNodes; ++i) {
        mergedMap[i] = currentMap[i] || receivedMap[i];
    }
    return mergedMap;
}

bool CarProbeRouting::hasNewInformation(int targetNodeId, const std::vector<bool>& receivedBitmap) {
    return routingTable.count(targetNodeId) == 0 || routingTable[targetNodeId].empty();
}


void CarProbeRouting::handleSelfMsg(cMessage* msg) {
    BaseWaveApplLayer::handleSelfMsg(msg);
}

void CarProbeRouting::handlePositionUpdate(cObject* obj) {
    BaseWaveApplLayer::handlePositionUpdate(obj);
    myPosition = mobility->getPosition();
}

void CarProbeRouting::finish() {
    BaseWaveApplLayer::finish();
    EV << "Node " << myId << ": Finalizing simulation. Routing table entries:" << endl;
    for(const auto& pair : routingTable) {
        std::string neighborsStr = "";
        for(int neighbor : pair.second) {
            neighborsStr += std::to_string(neighbor) + " ";
        }
         EV << "  Target " << pair.first << " -> via Neighbors: [ " << neighborsStr << "]" << endl;
    }
}
