import unreal


INPUT_DIR = "/Game/Data/Input"
TRANSFORM_DATA_DIR = "/Game/Data/Transform"
PICKUP_DIR = "/Game/Blueprints/Pickups/Transform"
UI_DIR = "/Game/UI"
PLAYER_CONTROLLER_BP = "/Game/Blueprints/Player/BP_GridPlayerController"
GRID_MOVEMENT_CONTEXT = "/Game/Data/Input/IMC_GridMovement"
DUNGEON_SETTINGS = "/Game/Data/Grid/DA_DungeonGenerationSettings"


TRANSFORMS = {
    "Knight": {
        "display_name": "Knight",
        "directions": [(1, 2), (2, 1), (2, -1), (1, -2), (-1, -2), (-2, -1), (-2, 1), (-1, 2)],
        "max_range": 1,
        "can_jump": True,
    },
    "Bishop": {
        "display_name": "Bishop",
        "directions": [(1, 1), (1, -1), (-1, 1), (-1, -1)],
        "max_range": 99,
        "can_jump": False,
    },
    "Rook": {
        "display_name": "Rook",
        "directions": [(1, 0), (-1, 0), (0, 1), (0, -1)],
        "max_range": 99,
        "can_jump": False,
    },
    "Queen": {
        "display_name": "Queen",
        "directions": [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1)],
        "max_range": 99,
        "can_jump": False,
    },
}


INPUT_ACTIONS = {
    "IA_OpenTransformWheel": "G",
    "IA_TransformLeftClick": "LeftMouseButton",
    "IA_TransformCancel": "Escape",
    "IA_TransformRightMouse": "RightMouseButton",
}


def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def load_class(path):
    cls = unreal.load_object(None, path)
    if not cls:
        raise RuntimeError("Class not found: {0}".format(path))
    return cls


def enum_value(enum_class, name):
    for candidate in (name.upper(), name, name.capitalize()):
        if hasattr(enum_class, candidate):
            return getattr(enum_class, candidate)
    raise RuntimeError("Enum value not found: {0}.{1}".format(enum_class, name))


def transform_enum_value(name):
    for enum_class_name in ("EChessTransformType", "ChessTransformType"):
        enum_class = getattr(unreal, enum_class_name, None)
        if not enum_class:
            continue
        try:
            return enum_value(enum_class, name)
        except Exception:
            pass

    numeric_values = {
        "None": 0,
        "Knight": 1,
        "Bishop": 2,
        "Rook": 3,
        "Queen": 4,
    }
    return numeric_values[name]


def set_property_safely(obj, prop_name, value):
    candidates = [
        prop_name,
        prop_name[0].lower() + prop_name[1:],
    ]
    for candidate in candidates:
        try:
            obj.set_editor_property(candidate, value)
            return True
        except Exception:
            pass
    return False


def make_int_point(x, y):
    return unreal.IntPoint(x, y)


def make_key(key_name):
    key = unreal.Key()
    for prop_name in ("KeyName", "key_name"):
        try:
            key.set_editor_property(prop_name, key_name)
            return key
        except Exception:
            pass
    raise RuntimeError("Failed to create key: {0}".format(key_name))


def get_generated_class(blueprint):
    generated_class_attr = getattr(blueprint, "generated_class", None)
    if callable(generated_class_attr):
        return generated_class_attr()
    return blueprint.get_editor_property("GeneratedClass")


def compile_blueprint_if_possible(blueprint):
    for utility_name in ("KismetEditorUtilities", "BlueprintEditorLibrary"):
        utility = getattr(unreal, utility_name, None)
        if utility and hasattr(utility, "compile_blueprint"):
            try:
                utility.compile_blueprint(blueprint)
                return True
            except Exception:
                pass
    return False


def save(asset_path):
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)


def create_blueprint(asset_path, parent_class):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    package_path, asset_name = asset_path.rsplit("/", 1)
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        package_path,
        unreal.Blueprint,
        factory,
    )
    if not blueprint:
        raise RuntimeError("Failed to create blueprint: {0}".format(asset_path))
    save(asset_path)
    return blueprint


def create_widget_blueprint(asset_path, parent_class):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    package_path, asset_name = asset_path.rsplit("/", 1)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    widget_factory_class = getattr(unreal, "WidgetBlueprintFactory", None)
    if widget_factory_class:
        factory = widget_factory_class()
        for prop_name in ("ParentClass", "parent_class"):
            try:
                factory.set_editor_property(prop_name, parent_class)
                break
            except Exception:
                pass
        widget_bp_class = getattr(unreal, "WidgetBlueprint", unreal.Blueprint)
        widget = asset_tools.create_asset(asset_name, package_path, widget_bp_class, factory)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("ParentClass", parent_class)
        widget = asset_tools.create_asset(asset_name, package_path, unreal.Blueprint, factory)

    if not widget:
        raise RuntimeError("Failed to create widget blueprint: {0}".format(asset_path))
    save(asset_path)
    return widget


def create_data_asset(asset_path, data_class):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    package_path, asset_name = asset_path.rsplit("/", 1)
    factory = unreal.DataAssetFactory()
    for prop_name in ("DataAssetClass", "data_asset_class"):
        try:
            factory.set_editor_property(prop_name, data_class)
            break
        except Exception:
            pass

    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, package_path, data_class, factory)
    if not asset:
        raise RuntimeError("Failed to create data asset: {0}".format(asset_path))
    save(asset_path)
    return asset


def create_input_action(asset_path):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        action = unreal.EditorAssetLibrary.load_asset(asset_path)
    else:
        package_path, asset_name = asset_path.rsplit("/", 1)
        input_action_factory_class = getattr(unreal, "InputActionFactory", None)
        factory = input_action_factory_class() if input_action_factory_class else None
        action = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name,
            package_path,
            unreal.InputAction,
            factory,
        )
        if not action:
            raise RuntimeError("Failed to create input action: {0}".format(asset_path))

    value_type_enum = getattr(unreal, "InputActionValueType", None)
    if value_type_enum:
        for candidate in ("BOOLEAN", "DIGITAL", "AXIS1D"):
            if hasattr(value_type_enum, candidate):
                try:
                    action.set_editor_property("ValueType", getattr(value_type_enum, candidate))
                    break
                except Exception:
                    try:
                        action.set_editor_property("value_type", getattr(value_type_enum, candidate))
                        break
                    except Exception:
                        pass
    save(asset_path)
    return action


def map_key_if_missing(mapping_context, action, key_name):
    key = make_key(key_name)
    try:
        mappings = mapping_context.get_editor_property("Mappings")
    except Exception:
        mappings = mapping_context.get_editor_property("mappings")

    for mapping in mappings:
        try:
            existing_action = mapping.get_editor_property("Action")
            existing_key = mapping.get_editor_property("Key")
        except Exception:
            existing_action = mapping.get_editor_property("action")
            existing_key = mapping.get_editor_property("key")
        if existing_action == action and str(existing_key) == str(key):
            return

    if hasattr(mapping_context, "map_key"):
        mapping_context.map_key(action, key)
    else:
        new_mapping = unreal.EnhancedActionKeyMapping()
        new_mapping.set_editor_property("Action", action)
        new_mapping.set_editor_property("Key", key)
        mappings.append(new_mapping)
        try:
            mapping_context.set_editor_property("Mappings", mappings)
        except Exception:
            mapping_context.set_editor_property("mappings", mappings)


def configure_form_data(asset, transform_name):
    data = TRANSFORMS[transform_name]
    if not set_property_safely(asset, "TransformType", transform_enum_value(transform_name)):
        unreal.log_warning("Failed to set TransformType on {0}".format(asset.get_name()))
    set_property_safely(asset, "DisplayName", unreal.Text(data["display_name"]))
    set_property_safely(asset, "MoveDirections", [make_int_point(x, y) for x, y in data["directions"]])
    set_property_safely(asset, "MaxRange", data["max_range"])
    set_property_safely(asset, "bCanJump", data["can_jump"])
    set_property_safely(asset, "bCanCaptureEnemy", True)


def configure_pickup_blueprint(blueprint, transform_name, asset_path):
    generated_class = get_generated_class(blueprint)
    if not generated_class:
        unreal.log_warning("Blueprint is not compiled yet: {0}".format(asset_path))
        return

    default_object = unreal.get_default_object(generated_class)
    set_property_safely(default_object, "TransformType", transform_enum_value(transform_name))
    set_property_safely(default_object, "Amount", 1)

    pickup_mesh = default_object.get_editor_property("PickupMesh")
    mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Sphere")
    if pickup_mesh and mesh:
        pickup_mesh.set_static_mesh(mesh)
        pickup_mesh.set_relative_scale3d(unreal.Vector(0.35, 0.35, 0.35))

    save(asset_path)


def add_pickups_to_dungeon_settings(pickup_blueprints):
    settings = unreal.EditorAssetLibrary.load_asset(DUNGEON_SETTINGS)
    if not settings:
        unreal.log_warning("Dungeon settings asset not found: {0}".format(DUNGEON_SETTINGS))
        return

    try:
        spawn_pool = list(settings.get_editor_property("PickupSpawnPool"))
    except Exception:
        spawn_pool = list(settings.get_editor_property("pickup_spawn_pool"))

    existing_classes = set()
    for entry in spawn_pool:
        try:
            pickup_class = entry.get_editor_property("PickupClass")
        except Exception:
            pickup_class = entry.get_editor_property("pickup_class")
        if pickup_class:
            existing_classes.add(pickup_class)

    for blueprint in pickup_blueprints:
        generated_class = get_generated_class(blueprint)
        if not generated_class or generated_class in existing_classes:
            continue

        entry = unreal.DungeonPickupSpawnEntry()
        entry.set_editor_property("PickupClass", generated_class)
        entry.set_editor_property("Weight", 1)
        entry.set_editor_property("MinDepth", 0)
        entry.set_editor_property("MaxDepth", 999)
        spawn_pool.append(entry)
        existing_classes.add(generated_class)

    settings.set_editor_property("PickupSpawnPool", spawn_pool)
    save(DUNGEON_SETTINGS)


def configure_player_controller(form_assets, input_actions, wheel_widget):
    controller_bp = unreal.EditorAssetLibrary.load_asset(PLAYER_CONTROLLER_BP)
    if not controller_bp:
        unreal.log_warning("Player controller blueprint not found: {0}".format(PLAYER_CONTROLLER_BP))
        return

    generated_class = get_generated_class(controller_bp)
    if not generated_class:
        compile_blueprint_if_possible(controller_bp)
        generated_class = get_generated_class(controller_bp)

    default_object = unreal.get_default_object(generated_class)
    default_object.set_editor_property("OpenTransformWheelAction", input_actions["IA_OpenTransformWheel"])
    default_object.set_editor_property("TransformLeftClickAction", input_actions["IA_TransformLeftClick"])
    default_object.set_editor_property("TransformCancelAction", input_actions["IA_TransformCancel"])
    default_object.set_editor_property("TransformRightMouseAction", input_actions["IA_TransformRightMouse"])
    default_object.set_editor_property("TransformWheelForms", form_assets)

    wheel_class = get_generated_class(wheel_widget)
    if wheel_class:
        default_object.set_editor_property("TransformWheelWidgetClass", wheel_class)

    compile_blueprint_if_possible(controller_bp)
    save(PLAYER_CONTROLLER_BP)


def main():
    ensure_dir(TRANSFORM_DATA_DIR)
    ensure_dir(PICKUP_DIR)
    ensure_dir(UI_DIR)

    form_class = load_class("/Script/Chessboard_Roguelike.ChessPieceFormData")
    pickup_parent_class = load_class("/Script/Chessboard_Roguelike.GridTransformPiecePickupActor")
    wheel_parent_class = load_class("/Script/Chessboard_Roguelike.TransformWheelWidget")

    form_assets = []
    for transform_name in ("Knight", "Bishop", "Rook", "Queen"):
        asset_path = "{0}/DA_Form_{1}".format(TRANSFORM_DATA_DIR, transform_name)
        form = create_data_asset(asset_path, form_class)
        configure_form_data(form, transform_name)
        save(asset_path)
        form_assets.append(form)
        unreal.log("Configured transform form data: {0}".format(asset_path))

    pickup_blueprints = []
    for transform_name in ("Knight", "Bishop", "Rook", "Queen"):
        asset_path = "{0}/BP_TransformPiece_{1}".format(PICKUP_DIR, transform_name)
        blueprint = create_blueprint(asset_path, pickup_parent_class)
        compile_blueprint_if_possible(blueprint)
        configure_pickup_blueprint(blueprint, transform_name, asset_path)
        pickup_blueprints.append(blueprint)
        unreal.log("Configured transform pickup blueprint: {0}".format(asset_path))

    wheel_widget = create_widget_blueprint("{0}/WBP_TransformWheel".format(UI_DIR), wheel_parent_class)
    compile_blueprint_if_possible(wheel_widget)
    save("{0}/WBP_TransformWheel".format(UI_DIR))
    unreal.log("Configured transform wheel widget blueprint: {0}/WBP_TransformWheel".format(UI_DIR))

    input_actions = {}
    for action_name, key_name in INPUT_ACTIONS.items():
        action_path = "{0}/{1}".format(INPUT_DIR, action_name)
        action = create_input_action(action_path)
        input_actions[action_name] = action
        unreal.log("Configured input action: {0}".format(action_path))

    mapping_context = unreal.EditorAssetLibrary.load_asset(GRID_MOVEMENT_CONTEXT)
    if not mapping_context:
        raise RuntimeError("Input mapping context not found: {0}".format(GRID_MOVEMENT_CONTEXT))
    for action_name, key_name in INPUT_ACTIONS.items():
        map_key_if_missing(mapping_context, input_actions[action_name], key_name)
        unreal.log("Mapped {0} to {1}".format(action_name, key_name))
    save(GRID_MOVEMENT_CONTEXT)

    configure_player_controller(form_assets, input_actions, wheel_widget)
    add_pickups_to_dungeon_settings(pickup_blueprints)

    unreal.EditorAssetLibrary.save_directory(TRANSFORM_DATA_DIR)
    unreal.EditorAssetLibrary.save_directory(PICKUP_DIR)
    unreal.EditorAssetLibrary.save_directory(UI_DIR)
    unreal.EditorAssetLibrary.save_directory(INPUT_DIR)
    unreal.log("Transform asset creation complete.")


main()
