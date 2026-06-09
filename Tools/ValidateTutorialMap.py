import os
import unreal


EXPECTED_DATA_ASSET = "/Game/Data/Tutorial/DA_TutorialLevelSet"


def find_dungeon_run_managers():
    managers = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if "DungeonRunManager" in actor.get_class().get_name():
            managers.append(actor)
    return managers


def main():
    expected_index = int(os.environ["CHESSBOARD_TUTORIAL_INDEX"])
    expected_level_set = unreal.EditorAssetLibrary.load_asset(EXPECTED_DATA_ASSET)
    if not expected_level_set:
        raise RuntimeError(f"Missing tutorial level set: {EXPECTED_DATA_ASSET}")

    managers = find_dungeon_run_managers()
    if not managers:
        raise RuntimeError("No DungeonRunManager actor found")

    for manager in managers:
        mode = manager.get_editor_property("generation_mode")
        level_set = manager.get_editor_property("tutorial_level_set")
        index = manager.get_editor_property("tutorial_level_index")
        if mode != unreal.DungeonRunGenerationMode.TUTORIAL_FIXED:
            raise RuntimeError(f"Unexpected generation mode: {mode}")
        if level_set != expected_level_set:
            raise RuntimeError(f"Unexpected tutorial level set: {level_set}")
        if index != expected_index:
            raise RuntimeError(f"Unexpected tutorial index: {index}, expected {expected_index}")

    unreal.log(f"Validated tutorial map index {expected_index} with {len(managers)} DungeonRunManager actor(s).")


if __name__ == "__main__":
    main()
