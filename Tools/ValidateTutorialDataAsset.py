import unreal


ASSET_PATH = "/Game/Data/Tutorial/DA_TutorialLevelSet"
EXPECTED_ENEMY_COUNTS = [0, 2, 3, 4, 2]


def get_tile_type_at(level, coord):
    for tile in level.get_editor_property("tiles"):
        if tile.get_editor_property("grid_coord") == coord:
            return tile.get_editor_property("tile_type")
    raise RuntimeError(f"Missing tile at {coord}")


def validate_faction_suppression_level(level):
    if level.get_editor_property("level_id") != "Tutorial_05_FactionSuppression":
        raise RuntimeError("Tutorial level 4 has an unexpected LevelId")
    if level.get_editor_property("start_coord") != unreal.IntPoint(5, 5):
        raise RuntimeError("Tutorial level 4 has an unexpected player start coord")

    for x in range(10):
        for y in (0, 1):
            if get_tile_type_at(level, unreal.IntPoint(x, y)) != unreal.TileType.ACID:
                raise RuntimeError(f"Tutorial level 4 expected Acid tile at ({x},{y})")
        for y in (8, 9):
            if get_tile_type_at(level, unreal.IntPoint(x, y)) != unreal.TileType.CONSTRUCT:
                raise RuntimeError(f"Tutorial level 4 expected Construct tile at ({x},{y})")

    enemies = list(level.get_editor_property("enemies"))
    enemy_coords = [enemy.get_editor_property("coord") for enemy in enemies]
    if unreal.IntPoint(0, 2) not in enemy_coords:
        raise RuntimeError("Tutorial level 4 is missing the Acid melee enemy at (0,2)")
    if unreal.IntPoint(0, 9) not in enemy_coords:
        raise RuntimeError("Tutorial level 4 is missing the Construct melee enemy at (0,9)")


def main():
    asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if not asset:
        raise RuntimeError(f"Missing tutorial level set: {ASSET_PATH}")

    levels = list(asset.get_editor_property("tutorial_levels"))
    if len(levels) != 5:
        raise RuntimeError(f"Expected 5 tutorial levels, found {len(levels)}")

    for index, level in enumerate(levels):
        width = level.get_editor_property("width")
        height = level.get_editor_property("height")
        tiles = list(level.get_editor_property("tiles"))
        enemies = list(level.get_editor_property("enemies"))
        if width != 10 or height != 10 or len(tiles) != 100:
            raise RuntimeError(f"Tutorial level {index} is not a 10x10 layout with 100 tiles")
        if len(enemies) != EXPECTED_ENEMY_COUNTS[index]:
            raise RuntimeError(f"Tutorial level {index} enemy count {len(enemies)} != {EXPECTED_ENEMY_COUNTS[index]}")
        for enemy in enemies:
            if not enemy.get_editor_property("enemy_class"):
                raise RuntimeError(f"Tutorial level {index} has an enemy without an enemy class")

    validate_faction_suppression_level(levels[4])

    unreal.log("Validated tutorial DataAsset: 5 levels, 10x10 layouts, configured enemy classes.")


if __name__ == "__main__":
    main()
