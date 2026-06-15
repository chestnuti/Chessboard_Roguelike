import uuid

import unreal


MPC_PATH = "/Game/Materials/MPC/MPC_PlayerPosition"
PLAYER_CONTROLLER_BP = "/Game/Blueprints/Player/BP_GridPlayerController"

HOVER_SCALAR_PARAMETERS = {
    "MouseHoverGridX": 0.0,
    "MouseHoverGridY": 0.0,
    "bHasMouseHoverGrid": 0.0,
}


def get_editor_property_safely(obj, *property_names):
    for property_name in property_names:
        try:
            return obj.get_editor_property(property_name)
        except Exception:
            pass
    raise RuntimeError("None of these properties are readable on {0}: {1}".format(obj, property_names))


def set_editor_property_safely(obj, value, *property_names):
    for property_name in property_names:
        try:
            obj.set_editor_property(property_name, value)
            return True
        except Exception:
            pass
    return False


def make_guid():
    guid_value = uuid.uuid4().int
    guid = unreal.Guid()
    parts = (
        (guid_value >> 96) & 0xFFFFFFFF,
        (guid_value >> 64) & 0xFFFFFFFF,
        (guid_value >> 32) & 0xFFFFFFFF,
        guid_value & 0xFFFFFFFF,
    )

    for prop_name, part in zip(("A", "B", "C", "D"), parts):
        set_editor_property_safely(guid, part, prop_name, prop_name.lower())

    return guid


def get_parameter_name(parameter):
    raw_name = get_editor_property_safely(parameter, "ParameterName", "parameter_name")
    return str(raw_name)


def set_parameter_name(parameter, parameter_name):
    if not set_editor_property_safely(parameter, parameter_name, "ParameterName", "parameter_name"):
        raise RuntimeError("Failed to set parameter name: {0}".format(parameter_name))


def set_parameter_default(parameter, default_value):
    if not set_editor_property_safely(parameter, default_value, "DefaultValue", "default_value"):
        raise RuntimeError("Failed to set parameter default: {0}".format(default_value))


def set_parameter_guid_if_possible(parameter):
    set_editor_property_safely(parameter, make_guid(), "Id", "id")


def create_scalar_parameter(parameter_name, default_value):
    scalar_parameter_class = getattr(unreal, "CollectionScalarParameter", None)
    if not scalar_parameter_class:
        raise RuntimeError("unreal.CollectionScalarParameter is not available in this editor Python environment.")

    parameter = scalar_parameter_class()
    set_parameter_name(parameter, parameter_name)
    set_parameter_default(parameter, default_value)
    set_parameter_guid_if_possible(parameter)
    return parameter


def ensure_mpc_parameters():
    mpc = unreal.EditorAssetLibrary.load_asset(MPC_PATH)
    if not mpc:
        raise RuntimeError("Material Parameter Collection not found: {0}".format(MPC_PATH))

    scalar_parameters = list(get_editor_property_safely(mpc, "ScalarParameters", "scalar_parameters"))
    existing_by_name = {get_parameter_name(parameter): parameter for parameter in scalar_parameters}

    changed = False
    for parameter_name, default_value in HOVER_SCALAR_PARAMETERS.items():
        if parameter_name in existing_by_name:
            set_parameter_default(existing_by_name[parameter_name], default_value)
            unreal.log("Updated MPC scalar default: {0}".format(parameter_name))
            changed = True
            continue

        scalar_parameters.append(create_scalar_parameter(parameter_name, default_value))
        unreal.log("Added MPC scalar parameter: {0}".format(parameter_name))
        changed = True

    if changed:
        if not set_editor_property_safely(mpc, scalar_parameters, "ScalarParameters", "scalar_parameters"):
            raise RuntimeError("Failed to write scalar parameter array to {0}".format(MPC_PATH))
        unreal.EditorAssetLibrary.save_asset(MPC_PATH, only_if_is_dirty=False)

    return mpc


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


def configure_player_controller_mpc(mpc):
    controller_bp = unreal.EditorAssetLibrary.load_asset(PLAYER_CONTROLLER_BP)
    if not controller_bp:
        unreal.log_warning("Player controller blueprint not found: {0}".format(PLAYER_CONTROLLER_BP))
        return

    generated_class = get_generated_class(controller_bp)
    if not generated_class:
        compile_blueprint_if_possible(controller_bp)
        generated_class = get_generated_class(controller_bp)
    if not generated_class:
        unreal.log_warning("Player controller blueprint has no generated class: {0}".format(PLAYER_CONTROLLER_BP))
        return

    default_object = unreal.get_default_object(generated_class)
    if set_editor_property_safely(default_object, mpc, "MouseHoverGridParameterCollection", "mouse_hover_grid_parameter_collection"):
        compile_blueprint_if_possible(controller_bp)
        unreal.EditorAssetLibrary.save_asset(PLAYER_CONTROLLER_BP, only_if_is_dirty=False)
        unreal.log("Configured controller MouseHoverGridParameterCollection: {0}".format(MPC_PATH))
    else:
        unreal.log_warning("Could not set MouseHoverGridParameterCollection on controller blueprint.")


def main():
    mpc = ensure_mpc_parameters()
    configure_player_controller_mpc(mpc)
    unreal.log("Mouse hover material parameter configuration complete.")


main()
