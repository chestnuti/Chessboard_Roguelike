import unreal


FLOW_DIR = "/Game/Data/Tutorial/UI"
TUTORIAL_LEVEL_SET = "/Game/Data/Tutorial/DA_TutorialLevelSet"


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_or_load_flow(asset_name):
    path = f"{FLOW_DIR}/{asset_name}"
    existing = unreal.EditorAssetLibrary.load_asset(path)
    if existing:
        return existing

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.TutorialFlowDataAsset.static_class())
    return asset_tools.create_asset(asset_name, FLOW_DIR, unreal.TutorialFlowDataAsset, factory)


def make_step(step_id, text, trigger, tile_type=None, attribute_kind=None, required_value=0, transform_type=None):
    step = unreal.TutorialStepDefinition()
    step.set_editor_property("step_id", step_id)
    step.set_editor_property("instruction_text", text)
    step.set_editor_property("trigger_type", trigger)
    if tile_type is not None:
        step.set_editor_property("target_tile_type", tile_type)
    if attribute_kind is not None:
        step.set_editor_property("attribute_kind", attribute_kind)
    step.set_editor_property("required_value", required_value)
    if transform_type is not None:
        step.set_editor_property("transform_type", transform_type)
    return step


def configure_flow(asset_name, flow_id, steps, next_level_name=""):
    flow = create_or_load_flow(asset_name)
    flow.set_editor_property("flow_id", flow_id)
    flow.set_editor_property("steps", steps)
    flow.set_editor_property("next_level_name", next_level_name)
    unreal.EditorAssetLibrary.save_loaded_asset(flow)
    return flow


def build_flows():
    trigger = unreal.TutorialStepTriggerType
    tile = unreal.TileType
    attr = unreal.TutorialAttributeKind

    return [
        configure_flow(
            "DA_TutorialFlow_01_TileAttributes",
            "TutorialFlow_01_TileAttributes",
            [
                make_step(
                    "Step_01_ConstructTile",
                    "Move onto a Construct tile and watch your attributes change. Construct tiles grant 1 Construct and remove 1 Acid.",
                    trigger.PLAYER_STEPPED_ON_TILE_TYPE,
                    tile_type=tile.CONSTRUCT,
                ),
                make_step(
                    "Step_02_AcidTile",
                    "Move onto an Acid tile and watch your attributes change. Acid tiles grant 1 Acid and remove 1 Construct.",
                    trigger.PLAYER_STEPPED_ON_TILE_TYPE,
                    tile_type=tile.ACID,
                ),
                make_step(
                    "Step_03_ClearAttributeTiles",
                    "Attribute tiles lose their power after you step on them. Clear every Construct and Acid tile.",
                    trigger.ALL_ATTRIBUTE_TILES_CLEARED,
                ),
            ],
            next_level_name="L_Tutorial_02_EnemyKills",
        ),
        configure_flow(
            "DA_TutorialFlow_02_EnemyKills",
            "TutorialFlow_02_EnemyKills",
            [
                make_step(
                    "Step_01_DualAttributeDamage",
                    "You can deal both Construct and Acid damage. Your damage is based on your current Construct and Acid values.",
                    trigger.PLAYER_STEPPED_ON_ANY_ATTRIBUTE_TILE,
                ),
                make_step(
                    "Step_02_EnemyImmunity",
                    "Watch the number above each enemy. Construct enemies ignore Construct damage, and Acid enemies ignore Acid damage.",
                    trigger.PLAYER_DAMAGED_OR_ENEMY_KILLED,
                ),
                make_step(
                    "Step_03_EnemyAttackDamage",
                    "When an enemy hits you, you lose health and 1 point of the matching attribute.",
                    trigger.PLAYER_DAMAGED_OR_ENEMY_KILLED,
                ),
                make_step(
                    "Step_04_KillAllEnemies",
                    "Use attribute tiles to build enough damage and defeat every enemy.",
                    trigger.ALL_ENEMIES_CLEARED,
                ),
            ],
            next_level_name="L_Tutorial_03_ConversionEnergy",
        ),
        configure_flow(
            "DA_TutorialFlow_03_ConversionEnergy",
            "TutorialFlow_03_ConversionEnergy",
            [
                make_step(
                    "Step_01_GainConversionEnergy",
                    "Defeating an enemy grants one tile conversion energy.",
                    trigger.ENEMY_KILLED,
                ),
                make_step(
                    "Step_02_UseConversionEnergy",
                    "Press E to switch the energy type. Hold Space to convert nearby tiles into the selected attribute.",
                    trigger.CONVERSION_ENERGY_USED,
                ),
                make_step(
                    "Step_03_KillAllEnemies",
                    "Use converted attribute tiles to build enough damage and defeat every enemy.",
                    trigger.ALL_ENEMIES_CLEARED,
                ),
            ],
            next_level_name="L_Tutorial_04_RangedFriendlyFire",
        ),
        configure_flow(
            "DA_TutorialFlow_04_RangedFriendlyFire",
            "TutorialFlow_04_RangedFriendlyFire",
            [
                make_step(
                    "Step_01_RangedWarning",
                    "Ranged enemies attack in a straight horizontal or vertical line. They spend one turn aiming before they fire.",
                    trigger.ENEMY_KILLED,
                ),
                make_step(
                    "Step_02_FriendlyFire",
                    "Ranged attacks can kill enemies from the opposing faction. Use friendly fire to clear the fight.",
                    trigger.ALL_ENEMIES_CLEARED,
                ),
            ],
            next_level_name="L_Tutorial_05_FactionSuppression",
        ),
        configure_flow(
            "DA_TutorialFlow_05_FactionSuppression",
            "TutorialFlow_05_FactionSuppression",
            [
                make_step(
                    "Step_01_AttributeMaxSuppression",
                    "When one of your attributes reaches its maximum value, enemies of the same faction stop acting.",
                    trigger.ATTRIBUTE_REACHED,
                    attribute_kind=attr.ANY,
                    required_value=10,
                ),
                make_step(
                    "Step_02_UseSuppression",
                    "Use faction suppression to avoid attacks from the matching enemy faction.",
                    trigger.ALL_ENEMIES_CLEARED,
                ),
            ],
            next_level_name="L_Tutorial_06_HealthAndTransform",
        ),
        configure_flow(
            "DA_TutorialFlow_06_HealthAndTransform",
            "TutorialFlow_06_HealthAndTransform",
            [
                make_step(
                    "Step_01_OpenTransformWheel",
                    "After collecting a transform card, press G to open the transform panel and choose a matching form.",
                    trigger.TRANSFORM_WHEEL_OPENED,
                ),
                make_step(
                    "Step_02_CollectAllTransformPieces",
                    "Transform effects let you make one special move. Using a form consumes one matching card.",
                    trigger.TRANSFORM_PIECE_COLLECTED_COUNT,
                    required_value=4,
                ),
            ],
        ),
    ]


def attach_flows_to_tutorial_levels(flows):
    level_set = unreal.EditorAssetLibrary.load_asset(TUTORIAL_LEVEL_SET)
    if not level_set:
        raise RuntimeError(f"Unable to load tutorial level set: {TUTORIAL_LEVEL_SET}")

    levels = list(level_set.get_editor_property("tutorial_levels"))
    if len(levels) < len(flows):
        raise RuntimeError(f"Expected at least {len(flows)} tutorial levels, found {len(levels)}")

    for index, flow in enumerate(flows):
        levels[index].set_editor_property("tutorial_flow", flow)

    level_set.set_editor_property("tutorial_levels", levels)
    unreal.EditorAssetLibrary.save_loaded_asset(level_set)


def main():
    ensure_directory(FLOW_DIR)
    flows = build_flows()
    attach_flows_to_tutorial_levels(flows)
    unreal.EditorAssetLibrary.save_directory(FLOW_DIR)
    unreal.log("Tutorial UI flow assets updated with English text and attached to the tutorial level set.")


if __name__ == "__main__":
    main()
