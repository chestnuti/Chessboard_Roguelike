import unreal


PROJECT_MAP_SOURCE = "/Game/Maps/L_GridMovement_Test"
TUTORIAL_DATA_DIR = "/Game/Data/Tutorial"
TUTORIAL_MAP_DIR = "/Game/Maps/Tutorial"
TUTORIAL_DATA_ASSET = f"{TUTORIAL_DATA_DIR}/DA_TutorialLevelSet"

TUTORIAL_MAPS = [
    ("L_Tutorial_01_TileAttributes", 0),
    ("L_Tutorial_02_EnemyKills", 1),
    ("L_Tutorial_03_ConversionEnergy", 2),
    ("L_Tutorial_04_RangedFriendlyFire", 3),
    ("L_Tutorial_05_FactionSuppression", 4),
]

ENEMY_CLASS_PATHS = {
    "construct_melee": "/Game/Blueprints/Enemies/ConstructEnemies/BP_ConstructEnemy_Melee.BP_ConstructEnemy_Melee_C",
    "acid_melee": "/Game/Blueprints/Enemies/AcidEnemies/BP_AcidEnemy_Melee.BP_AcidEnemy_Melee_C",
    "construct_ranged": "/Game/Blueprints/Enemies/ConstructEnemies/BP_ConstructEnemy_Ranged.BP_ConstructEnemy_Ranged_C",
    "acid_ranged": "/Game/Blueprints/Enemies/AcidEnemies/BP_AcidEnemy_Ranged.BP_AcidEnemy_Ranged_C",
}


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def load_class(path):
    loaded = unreal.load_object(None, path)
    if not loaded:
        raise RuntimeError(f"Unable to load class: {path}")
    return loaded


def make_enemy(coord, enemy_class, faction, behavior_type, kill_threshold, max_range=0):
    enemy = unreal.TutorialEnemySpawnData()
    enemy.set_editor_property("coord", coord)
    enemy.set_editor_property("enemy_class", enemy_class)
    enemy.set_editor_property("faction", faction)
    enemy.set_editor_property("behavior_type", behavior_type)
    enemy.set_editor_property("kill_threshold", kill_threshold)
    enemy.set_editor_property("max_ranged_attack_distance", max_range)
    return enemy


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


def make_default_tiles():
    tiles = []
    minimal = unreal.TileType.MINIMAL
    acid = unreal.TileType.ACID
    construct = unreal.TileType.CONSTRUCT

    for y in range(10):
        for x in range(10):
            tile_type = minimal
            if y in (0, 1):
                tile_type = acid
            elif y in (8, 9):
                tile_type = construct
            tiles.append(make_tile(unreal.IntPoint(x, y), tile_type))

    return tiles


def make_faction_suppression_level(acid_melee, construct_melee, acid, construct, melee):
    level = unreal.TutorialLevelDefinition()
    level.set_editor_property("level_id", "Tutorial_05_FactionSuppression")
    level.set_editor_property("width", 10)
    level.set_editor_property("height", 10)
    level.set_editor_property("start_coord", unreal.IntPoint(5, 5))
    level.set_editor_property("exit_coord", unreal.IntPoint(9, 5))
    level.set_editor_property("tiles", make_default_tiles())
    level.set_editor_property(
        "enemies",
        [
            make_enemy(unreal.IntPoint(0, 2), acid_melee, acid, melee, 2),
            make_enemy(unreal.IntPoint(0, 9), construct_melee, construct, melee, 2),
        ],
    )
    return level


def configure_tutorial_level_set(asset):
    levels = list(asset.get_editor_property("tutorial_levels"))
    construct_melee = load_class(ENEMY_CLASS_PATHS["construct_melee"])
    acid_melee = load_class(ENEMY_CLASS_PATHS["acid_melee"])
    construct_ranged = load_class(ENEMY_CLASS_PATHS["construct_ranged"])
    acid_ranged = load_class(ENEMY_CLASS_PATHS["acid_ranged"])

    construct = unreal.EnemyFaction.CONSTRUCT
    acid = unreal.EnemyFaction.ACID
    melee = unreal.EnemyBehaviorType.MELEE
    ranged = unreal.EnemyBehaviorType.RANGED

    while len(levels) < 5:
        levels.append(make_faction_suppression_level(acid_melee, construct_melee, acid, construct, melee))

    levels[0].set_editor_property("enemies", [])
    levels[1].set_editor_property(
        "enemies",
        [
            make_enemy(unreal.IntPoint(6, 2), construct_melee, construct, melee, 2),
            make_enemy(unreal.IntPoint(2, 6), acid_melee, acid, melee, 2),
        ],
    )
    levels[2].set_editor_property(
        "enemies",
        [
            make_enemy(unreal.IntPoint(3, 1), acid_melee, acid, melee, 1),
            make_enemy(unreal.IntPoint(8, 4), construct_melee, construct, melee, 3),
            make_enemy(unreal.IntPoint(8, 6), acid_melee, acid, melee, 3),
        ],
    )
    levels[3].set_editor_property(
        "enemies",
        [
            make_enemy(unreal.IntPoint(6, 2), construct_melee, construct, melee, 2),
            make_enemy(unreal.IntPoint(2, 6), acid_melee, acid, melee, 2),
            make_enemy(unreal.IntPoint(8, 1), construct_ranged, construct, ranged, 2, 8),
            make_enemy(unreal.IntPoint(1, 8), acid_ranged, acid, ranged, 2, 8),
        ],
    )
    levels[4].set_editor_property(
        "enemies",
        [
            make_enemy(unreal.IntPoint(0, 2), acid_melee, acid, melee, 2),
            make_enemy(unreal.IntPoint(0, 9), construct_melee, construct, melee, 2),
        ],
    )
    levels[4].set_editor_property("level_id", "Tutorial_05_FactionSuppression")
    levels[4].set_editor_property("width", 10)
    levels[4].set_editor_property("height", 10)
    levels[4].set_editor_property("start_coord", unreal.IntPoint(5, 5))
    levels[4].set_editor_property("exit_coord", unreal.IntPoint(9, 5))
    levels[4].set_editor_property("tiles", make_default_tiles())

    asset.set_editor_property("tutorial_levels", levels)
    unreal.EditorAssetLibrary.save_loaded_asset(asset)


def create_or_load_level_set():
    ensure_directory(TUTORIAL_DATA_DIR)
    existing = unreal.EditorAssetLibrary.load_asset(TUTORIAL_DATA_ASSET)
    if existing:
        asset = existing
    else:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.TutorialLevelSet.static_class())
        asset = asset_tools.create_asset(
            "DA_TutorialLevelSet",
            TUTORIAL_DATA_DIR,
            unreal.TutorialLevelSet,
            factory,
        )
    configure_tutorial_level_set(asset)
    return asset


def find_dungeon_run_managers():
    managers = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_class().is_child_of(unreal.DungeonRunManager):
            managers.append(actor)
    return managers


def configure_current_map(level_set, tutorial_index):
    managers = find_dungeon_run_managers()
    if not managers:
        raise RuntimeError("No DungeonRunManager actor found in duplicated tutorial map")

    for manager in managers:
        manager.set_editor_property("generation_mode", unreal.DungeonRunGenerationMode.TUTORIAL_FIXED)
        manager.set_editor_property("tutorial_level_set", level_set)
        manager.set_editor_property("tutorial_level_index", tutorial_index)
        manager.set_editor_property("spawn_enemies", False)
        manager.set_editor_property("spawn_pickups", False)
    unreal.EditorLevelLibrary.set_level_dirty(unreal.EditorLevelLibrary.get_editor_world())


def create_tutorial_maps(level_set):
    ensure_directory(TUTORIAL_MAP_DIR)

    for map_name, tutorial_index in TUTORIAL_MAPS:
        target_path = f"{TUTORIAL_MAP_DIR}/{map_name}"
        if unreal.EditorAssetLibrary.does_asset_exist(target_path):
            unreal.EditorAssetLibrary.delete_asset(target_path)

        if not unreal.EditorAssetLibrary.duplicate_asset(PROJECT_MAP_SOURCE, target_path):
            raise RuntimeError(f"Failed to duplicate {PROJECT_MAP_SOURCE} to {target_path}")
        unreal.EditorAssetLibrary.save_asset(target_path)


def main():
    level_set = create_or_load_level_set()
    create_tutorial_maps(level_set)
    unreal.EditorAssetLibrary.save_directory(TUTORIAL_DATA_DIR)
    unreal.EditorAssetLibrary.save_directory(TUTORIAL_MAP_DIR)
    unreal.log("Tutorial maps and data asset created successfully.")


if __name__ == "__main__":
    main()
