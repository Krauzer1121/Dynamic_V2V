#include <omnetpp.h>
#include "veins/base/modules/BaseApplLayer.h"
#include "veins/base/utils/Coord.h"
#include "veins/modules/messages/DemoSafetyMessage_m.h"

using namespace omnetpp;
using namespace veins;

class CarJamInet : public BaseApplLayer {
  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    void checkTrafficJam();
};

Define_Module(CarJamInet);

void CarJamInet::initialize(int stage) {
    BaseApplLayer::initialize(stage);
    if (stage == 0) {
        // Инициализация
    }
}

void CarJamInet::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        checkTrafficJam();
    }
    delete msg;
}

void CarJamInet::checkTrafficJam() {
    double speedThreshold = 0.1; // Пороговая скорость для определения пробки

    DemoSafetyMessage *demoMsg = new DemoSafetyMessage();
    Coord senderSpeed = demoMsg->getSenderSpeed();
    if (senderSpeed.x <= speedThreshold && senderSpeed.y <= speedThreshold && senderSpeed.z <= speedThreshold) {
        // Машина стоит, отправляем сигнал в сеть о пробке
        EV << "Sending traffic jam signal to VANET network" << endl;
        // Отправляем сообщение о пробке в сеть
        send(demoMsg, "out");
    } else {
        delete demoMsg;
    }
}
