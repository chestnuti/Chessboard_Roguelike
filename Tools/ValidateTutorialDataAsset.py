import unreal


ASSET_PATH = "/Game/Data/Tutorial/DA_TutorialLevelSet"
EXPECTED_LEVEL_06_ID = "Tutorial_06_HealthAndTransform"
EXPECTED_LEVEL_06_PICKUPS = {
    (2, 1): "BP_HealthPickup_C",
    (1, 3): "BP_HealthPickup_C",
    (7, 7): "BP_HealthPickup_C",
    (3, 2): "BP_TransformPiece_Knight_C",
    (5, 2): "BP_TransformPiece_Bishop_C",
    (3, 5): "BP_TransformPiece_Rook_C",
    (6, 6): "BP_TransformPiece_Queen_C",
}


def coord_tuple(coord):
    return (coord.x, coord.y)


def validate_level_shape(level, index):
    width = level.get_editor_property("width")
    height = level.get_editor_property("height")
    tiles = list(level.get_editor_property("tiles"))
    if width != 10 or height != 10 or len(tiles) != 100:
        raise RuntimeError(f"Tutorial level {index} is not a 10x10 layout with 100 tiles")


def validate_level_06(level):
    if level.get_editor_property("level_id") != EXPECTED_LEVEL_06_ID:
        raise RuntimeError("Tutorial level 6 has an unexpected LevelId")
    if level.get_editor_property("start_coord") != unreal.IntPoint(1, 1):
        raise RuntimeError("Tutorial level 6 has an unexpected player start coord")
    if level.get_editor_property("exit_coord") != unreal.IntPoint(8, 8):
        raise RuntimeError("Tutorial level 6 has an unexpected exit coord")

    pickups = list(level.get_editor_property("pickups"))
    if len(pickups) != len(EXPECTED_LEVEL_06_PICKUPS):
        raise RuntimeError(
            f"Tutorial level 6 pickup count {len(pickups)} != {len(EXPECTED_LEVEL_06_PICKUPS)}"
        )

    found = {}
    for pickup in pickups:
        coord = coord_tuple(pickup.get_editor_property("coord"))
        pickup_class = pickup.get_editor_property("pickup_class")
        if not pickup_class:
            raise RuntimeError(f"Tutorial level 6 has a pickup without a class at {coord}")
        found[coord] = pickup_class.get_name()

    if set(found.keys()) != set(EXPECTED_LEVEL_06_PICKUPS.keys()):
        raise RuntimeError(f"Tutorial level 6 pickup coords mismatch: {sorted(found.keys())}")

    for coord, expected_class_name in EXPECTED_LEVEL_06_PICKUPS.items():
        if found[coord] != expected_class_name:
            raise RuntimeError(
                f"Tutorial level 6 pickup at {coord} uses {found[coord]}, expected {expected_class_name}"
            )


def main():
    asset = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if not asset:
        raise RuntimeError(f"Missing tutorial level set: {ASSET_PATH}")

    levels = list(asset.get_editor_property("tutorial_levels"))
    if len(levels) < 6:
        raise RuntimeError(f"Expected at least 6 tutorial levels, found {len(levels)}")

    validate_level_shape(levels[5], 5)
    validate_level_06(levels[5])

    unreal.log("Validated tutorial DataAsset: level 6 fixed layout and pickup classes are configured.")


if __name__ == "__main__":
    main()
