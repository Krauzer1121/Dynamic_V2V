import traci
import sumolib
import random

SUMO_CONFIG = "manhattan.sumocfg"       
NET_FILE    = "manhattan.net.xml"       
REROUTE_PROB = 0.3                      
ALLOWED_TYPES = ["rerouteCar", "bus"]   
MAX_SIM_STEPS = 10000                   
REROUTE_LENGTH = 5                      

def get_random_route(net, start_edge, length=5):
    """
    Генерирует случайный маршрут из 'length' рёбер, начиная с 'start_edge'.
    Возвращает список ID рёбер (маршрут).
    """
    route = [start_edge]
    current_edge = net.getEdge(start_edge)
    
    for _ in range(length - 1):
        outgoing = list(current_edge.getOutgoing().keys()) 
        if not outgoing:
            break
        # Выбираем случайное исходящее ребро
        next_edge = random.choice(outgoing)
        route.append(next_edge.getID())
        current_edge = next_edge
    
    return route

def run_simulation():
    # Запускаем SUMO
    traci.start([ "sumo-gui", "-c", SUMO_CONFIG ])
    
    net = sumolib.net.readNet(NET_FILE)

    step = 0
    rerouted_vehicles = 0  
    while step < MAX_SIM_STEPS:
        traci.simulationStep()
        vehicle_ids = traci.vehicle.getIDList()
        
        for vid in vehicle_ids:
            vtype = traci.vehicle.getTypeID(vid)
            if vtype in ALLOWED_TYPES:
                if random.random() < REROUTE_PROB:
                    # Получаем текущее ребро
                    current_edge_id = traci.vehicle.getRoadID(vid)
                    
                    new_route = get_random_route(net, current_edge_id, length=REROUTE_LENGTH)
                    
                    if len(new_route) > 1:
                        traci.vehicle.setRoute(vid, new_route)
                        rerouted_vehicles += 1
                        print(f"Vehicle {vid} rerouted to {new_route}")

        step += 1
    print(f"Total rerouted vehicles: {rerouted_vehicles}")
    
    traci.close()

if __name__ == "__main__":
    run_simulation()
