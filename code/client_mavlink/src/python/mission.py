import asyncio
from mavsdk import System
from mavsdk.action import ActionError
from mavsdk.mission import MissionItem, MissionPlan

def build_mission_plan(waypoints: list[tuple[float, float, float]]) -> MissionPlan:
    mission_items = []
    for lat, lon, alt in waypoints:
        mission_items.append(MissionItem(
            lat, lon, alt,
            10,                                   # vitesse (m/s) sur ce segment
            True,                                 # is_fly_through: continue sans s'arrêter au point
            float('nan'),                         # gimbal_pitch (non utilisé)
            float('nan'),                         # gimbal_yaw (non utilisé)
            MissionItem.CameraAction.NONE,
            float('nan'),                         # loiter_time_s
            float('nan'),                         # camera_photo_interval_s
            float('nan'),                         # acceptance_radius_m (défaut si nan)
            float('nan'),                         # yaw_deg
            float('nan'),                         # camera_photo_distance_m
            MissionItem.VehicleAction.NONE
        ))
    return MissionPlan(mission_items)

async def run_mission(drone: System, waypoints: list[tuple[float, float, float]]):
    print("Upload de la mission...")
    plan = build_mission_plan(waypoints)
    await drone.mission.upload_mission(plan)
    print("Mission uploadée")

    print("Démarrage de la mission...")
    await drone.mission.start_mission()

    async for progress in drone.mission.mission_progress():
        print(f"  Waypoint {progress.current}/{progress.total}")
        if progress.current == progress.total:
            print("Mission terminée")
            break

async def return_to_launch(drone: System):
    print("Retour au point de décollage...")
    await drone.action.return_to_launch()

    async for in_air in drone.telemetry.in_air():
        if not in_air:
            print("Drone posé")
            break

async def connect_drone() -> System:
    drone = System()
    await drone.connect(system_address="udpin://0.0.0.0:14540")

    print("En attente de connexion au drone...")
    async for state in drone.core.connection_state():
        if state.is_connected:
            print("Drone connecté !")
            break

    print("En attente d'une position GPS valide...")
    async for health in drone.telemetry.health():
        if health.is_global_position_ok and health.is_home_position_ok:
            print("Position globale et home position OK")
            break

    return drone

async def arm_and_takeoff(drone: System, altitude: float = 10.0):
    print("Armement...")
    try:
        await drone.action.arm()
    except ActionError as e:
        print(f"Échec de l'armement: {e}")
        return
    print("Drone armé")

    print(f"Décollage à {altitude}m...")
    await drone.action.set_takeoff_altitude(altitude)
    await drone.action.takeoff()

    # Attendre que l'altitude cible soit atteinte
    async for position in drone.telemetry.position():
        print(f"  altitude relative = {position.relative_altitude_m:.2f} m")
        if position.relative_altitude_m >= altitude * 0.95:
            print("Altitude de décollage atteinte")
            break

async def main():
    drone = await connect_drone()
    await arm_and_takeoff(drone, altitude=10.0)

    waypoints = [
        # Ligne 1 (Sud -> Nord)
        (47.3970000, 8.5440000, 10.0),
        (47.3980000, 8.5440000, 10.0),
        (47.3990000, 8.5440000, 10.0),
        (47.4000000, 8.5440000, 10.0),

        # Ligne 2 (Nord -> Sud)
        (47.4000000, 8.5450000, 12.0),
        (47.3990000, 8.5450000, 12.0),
        (47.3980000, 8.5450000, 12.0),
        (47.3970000, 8.5450000, 12.0),

        # Ligne 3 (Sud -> Nord)
        (47.3970000, 8.5460000, 15.0),
        (47.3980000, 8.5460000, 15.0),
        (47.3990000, 8.5460000, 15.0),
        (47.4000000, 8.5460000, 15.0),

        # Ligne 4 (Nord -> Sud)
        (47.4000000, 8.5470000, 10.0),
        (47.3990000, 8.5470000, 10.0),
        (47.3980000, 8.5470000, 10.0),
        (47.3970000, 8.5470000, 10.0)
    ]
    await run_mission(drone, waypoints)
    await return_to_launch(drone)

if __name__ == "__main__":
    asyncio.run(main())