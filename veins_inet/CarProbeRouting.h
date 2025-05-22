#ifndef CARPROBEROUTING_H_
#define CARPROBEROUTING_H_

#include <omnetpp.h>
#include <vector>
#include <map>
#include "veins/modules/application/ieee80211p/BaseWaveApplLayer.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/messages/WaveShortMessage_m.h"
#include "ProbeMessage_m.h"

using namespace omnetpp;
using namespace veins;


class CarProbeRouting : public BaseWaveApplLayer {
public:
    virtual void initialize(int stage) override;
    virtual void finish() override;

protected:
    int numNodes;
    int startProbeNodeId;
    simtime_t probeInterval;

    std::map<int, std::vector<int>> routingTable;

    int myId;

    TraCIMobility* mobility;
    Coord myPosition;

    virtual void handleSelfMsg(cMessage* msg) override;
    virtual void handleLowerMsg(cMessage* msg) override;
    virtual void handlePositionUpdate(cObject* obj) override;

    void initializeRoutingTable();
    void startProbeRouting();
    void sendProbe(int destination = -1);
    void processProbe(ProbeMessage* probeMsg);
    bool updateRoutingTable(int targetNodeId, int neighborId, const std::vector<bool>& receivedBitmap);

    std::vector<bool> createInitialBitmap();
    std::vector<bool> mergeBitmaps(const std::vector<bool>& currentMap, const std::vector<bool>& receivedMap);
    bool hasNewInformation(int targetNodeId, const std::vector<bool>& receivedBitmap);
};

#endif /* CARPROBEROUTING_H_ */
