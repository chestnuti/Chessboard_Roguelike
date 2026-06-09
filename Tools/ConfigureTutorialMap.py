import os
import unreal


TUTORIAL_DATA_ASSET = "/Game/Data/Tutorial/DA_TutorialLevelSet"


def find_dungeon_run_managers():
    managers = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        class_name = actor.get_class().get_name()
        if "DungeonRunManager" in class_name:
            managers.append(actor)
    return managers


def main():
    tutorial_index_value = os.environ.get("CHESSBOARD_TUTORIAL_INDEX")
    if tutorial_index_value is None:
        raise RuntimeError("CHESSBOARD_TUTORIAL_INDEX is not set")

    tutorial_index = int(tutorial_index_value)
    level_set = unreal.EditorAssetLibrary.load_asset(TUTORIAL_DATA_ASSET)
    if not level_set:
        raise RuntimeError(f"Unable to load tutorial level set: {TUTORIAL_DATA_ASSET}")

    managers = find_dungeon_run_managers()
    if not managers:
        raise RuntimeError("No DungeonRunManager actor found in current map")

    for manager in managers:
        manager.set_editor_property("generation_mode", unreal.DungeonRunGenerationMode.TUTORIAL_FIXED)
        manager.set_editor_property("tutorial_level_set", level_set)
        manager.set_editor_property("tutorial_level_index", tutorial_index)
        manager.set_editor_property("spawn_enemies", False)
        manager.set_editor_property("spawn_pickups", False)

    unreal.EditorLevelLibrary.save_current_level()
    unreal.log(f"Configured tutorial map with tutorial index {tutorial_index}.")


if __name__ == "__main__":
    main()
