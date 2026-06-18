import unreal


TUTORIAL_LEVEL_SET = "/Game/Data/Tutorial/DA_TutorialLevelSet"
EXPECTED_STEP_COUNTS = [3, 4, 3, 2, 2, 2]
EXPECTED_NEXT_LEVELS = [
    "L_Tutorial_02_EnemyKills",
    "L_Tutorial_03_ConversionEnergy",
    "L_Tutorial_04_RangedFriendlyFire",
    "L_Tutorial_05_FactionSuppression",
    "L_Tutorial_06_HealthAndTransform",
    "None",
]


def main():
    level_set = unreal.EditorAssetLibrary.load_asset(TUTORIAL_LEVEL_SET)
    if not level_set:
        raise RuntimeError(f"Unable to load tutorial level set: {TUTORIAL_LEVEL_SET}")

    levels = list(level_set.get_editor_property("tutorial_levels"))
    if len(levels) < len(EXPECTED_STEP_COUNTS):
        raise RuntimeError(f"Expected at least {len(EXPECTED_STEP_COUNTS)} tutorial levels, found {len(levels)}")

    for index, expected_count in enumerate(EXPECTED_STEP_COUNTS):
        flow = levels[index].get_editor_property("tutorial_flow")
        if not flow:
            raise RuntimeError(f"Tutorial level {index + 1} has no tutorial flow assigned")

        steps = list(flow.get_editor_property("steps"))
        if len(steps) != expected_count:
            raise RuntimeError(
                f"Tutorial level {index + 1} flow step count {len(steps)} != {expected_count}"
            )

        next_level_name = str(flow.get_editor_property("next_level_name"))
        if next_level_name != EXPECTED_NEXT_LEVELS[index]:
            raise RuntimeError(
                f"Tutorial level {index + 1} next level {next_level_name} != {EXPECTED_NEXT_LEVELS[index]}"
            )

        for step_index, step in enumerate(steps):
            text = step.get_editor_property("instruction_text")
            if text.is_empty():
                raise RuntimeError(f"Tutorial level {index + 1} step {step_index + 1} has empty text")

    unreal.log("Validated tutorial UI flows: all 6 tutorial levels have configured step text.")


if __name__ == "__main__":
    main()
