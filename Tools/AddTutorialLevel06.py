import unreal


PROJECT_MAP_SOURCE = "/Game/Maps/L_GridMovement_Test"
TUTORIAL_DATA_ASSET = "/Game/Data/Tutorial/DA_TutorialLevelSet"
TUTORIAL_MAP = "/Game/Maps/Tutorial/L_Tutorial_06_HealthAndTransform"

PICKUP_CLASS_PATHS = {
    "health": "/Game/Blueprints/Pickups/BP_HealthPickup.BP_HealthPickup_C",
    "knight": "/Game/Blueprints/Pickups/Transform/BP_TransformPiece_Knight.BP_TransformPiece_Knight_C",
    "bishop": "/Game/Blueprints/Pickups/Transform/BP_TransformPiece_Bishop.BP_TransformPiece_Bishop_C",
    "rook": "/Game/Blueprints/Pickups/Transform/BP_TransformPiece_Rook.BP_TransformPiece_Rook_C",
    "queen": "/Game/Blueprints/Pickups/Transform/BP_TransformPiece_Queen.BP_TransformPiece_Queen_C",
}


def load_class(path):
    loaded = unreal.load_object(None, path)
    if not loaded:
        raise RuntimeError(f"Unable to load class: {path}")
    return loaded


def make_tile(coord, tile_type):
    tile = unreal.GridTileLayoutData()
    tile.set_editor_property("grid_coord", coord)
    tile.set_editor_property("tile_type", tile_type)
    tile.set_editor_property("cell_role", unreal.GridCellRole.OPEN)
    tile.set_editor_property("walkable", True)
    tile.set_editor_property("convertible", True)
    tile.set_editor_property("region_id", 0)
    tile.set_editor_property("depth", 0)
    return tile


def make_tiles():
    construct_coords = {
        (2, 1),
        (3, 2),
        (3, 5),
        (7, 7),
        (8, 8),
    }
    acid_coords = {
        (1, 3),
        (5, 2),
        (6, 6),
        (2, 7),
    }

    tiles = []
    for y in range(10):
        for x in range(10):
            tile_type = unreal.TileType.MINIMAL
            if (x, y) in construct_coords:
                tile_type = unreal.TileType.CONSTRUCT
            elif (x, y) in acid_coords:
                tile_type = unreal.TileType.ACID
            tiles.append(make_tile(unreal.IntPoint(x, y), tile_type))
    return tiles


def make_pickup(coord, pickup_class):
    pickup = unreal.TutorialPickupSpawnData()
    pickup.set_editor_property("coord", coord)
    pickup.set_editor_property("pickup_class", pickup_class)
    return pickup


def make_level(pickup_classes):
    level = unreal.TutorialLevelDefinition()
    level.set_editor_property("level_id", "Tutorial_06_HealthAndTransform")
    level.set_editor_property("width", 10)
    level.set_editor_property("height", 10)
    level.set_editor_property("start_coord", unreal.IntPoint(1, 1))
    level.set_editor_property("exit_coord", unreal.IntPoint(8, 8))
    level.set_editor_property("tiles", make_tiles())
    level.set_editor_property("enemies", [])
    level.set_editor_property(
        "pickups",
        [
            make_pickup(unreal.IntPoint(2, 1), pickup_classes["health"]),
            make_pickup(unreal.IntPoint(1, 3), pickup_classes["health"]),
            make_pickup(unreal.IntPoint(7, 7), pickup_classes["health"]),
            make_pickup(unreal.IntPoint(3, 2), pickup_classes["knight"]),
            make_pickup(unreal.IntPoint(5, 2), pickup_classes["bishop"]),
            make_pickup(unreal.IntPoint(3, 5), pickup_classes["rook"]),
            make_pickup(unreal.IntPoint(6, 6), pickup_classes["queen"]),
        ],
    )
    return level


def configure_tutorial_level_set():
    level_set = unreal.EditorAssetLibrary.load_asset(TUTORIAL_DATA_ASSET)
    if not level_set:
        raise RuntimeError(f"Unable to load tutorial level set: {TUTORIAL_DATA_ASSET}")

    levels = list(level_set.get_editor_property("tutorial_levels"))
    if len(levels) < 5:
        raise RuntimeError(
            f"Expected existing tutorial levels 1-5 before adding level 6, found {len(levels)}"
        )

    pickup_classes = {key: load_class(path) for key, path in PICKUP_CLASS_PATHS.items()}
    level_06 = make_level(pickup_classes)

    if len(levels) == 5:
        levels.append(level_06)
    else:
        levels[5] = level_06

    level_set.set_editor_property("tutorial_levels", levels)
    unreal.EditorAssetLibrary.save_loaded_asset(level_set)
    return level_set


def find_dungeon_run_managers():
    managers = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if "DungeonRunManager" in class_name:
            managers.append(actor)
    return managers


def create_or_load_tutorial_map(level_set):
    if not unreal.EditorAssetLibrary.does_asset_exist(TUTORIAL_MAP):
        if not unreal.EditorAssetLibrary.duplicate_asset(PROJECT_MAP_SOURCE, TUTORIAL_MAP):
            raise RuntimeError(f"Failed to duplicate {PROJECT_MAP_SOURCE} to {TUTORIAL_MAP}")
        unreal.EditorAssetLibrary.save_asset(TUTORIAL_MAP)

    if not unreal.EditorLoadingAndSavingUtils.load_map(TUTORIAL_MAP):
        raise RuntimeError(f"Unable to load tutorial map: {TUTORIAL_MAP}")

    managers = find_dungeon_run_managers()
    if not managers:
        raise RuntimeError("No DungeonRunManager actor found in tutorial level 6 map")

    for manager in managers:
        manager.set_editor_property("generation_mode", unreal.DungeonRunGenerationMode.TUTORIAL_FIXED)
        manager.set_editor_property("tutorial_level_set", level_set)
        manager.set_editor_property("tutorial_level_index", 5)
        manager.set_editor_property("spawn_enemies", False)
        manager.set_editor_property("spawn_pickups", False)

    unreal.EditorLevelLibrary.save_current_level()


def main():
    level_set = configure_tutorial_level_set()
    create_or_load_tutorial_map(level_set)
    unreal.log("Tutorial level 6 data and map configured successfully.")


if __name__ == "__main__":
    main()
