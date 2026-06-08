import unreal


def get_native_class(class_path):
    native_class = unreal.load_object(None, class_path)
    if not native_class:
        raise RuntimeError("Native class not found: {0}".format(class_path))
    return native_class


def create_blueprint(asset_path, parent_class):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log("Blueprint already exists: {0}".format(asset_path))
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    package_path, asset_name = asset_path.rsplit("/", 1)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("ParentClass", parent_class)

    blueprint = asset_tools.create_asset(asset_name, package_path, unreal.Blueprint, factory)
    if not blueprint:
        raise RuntimeError("Failed to create blueprint: {0}".format(asset_path))

    unreal.EditorAssetLibrary.save_asset(asset_path)
    unreal.log("Created blueprint: {0}".format(asset_path))
    return blueprint


def get_blueprint_generated_class(blueprint):
    generated_class_attr = getattr(blueprint, "generated_class", None)
    if callable(generated_class_attr):
        return generated_class_attr()
    return blueprint.get_editor_property("GeneratedClass")


def configure_health_pickup(blueprint, asset_path):
    generated_class = get_blueprint_generated_class(blueprint) if blueprint else None
    if not generated_class:
        unreal.log_warning("Health pickup blueprint is not compiled yet; skipping component defaults.")
        return

    default_object = unreal.get_default_object(generated_class)
    pickup_mesh = default_object.get_editor_property("PickupMesh")
    sphere_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Sphere")
    if pickup_mesh and sphere_mesh:
        pickup_mesh.set_static_mesh(sphere_mesh)
        pickup_mesh.set_relative_scale3d(unreal.Vector(0.35, 0.35, 0.35))
        unreal.EditorAssetLibrary.save_asset(asset_path)


def main():
    health_pickup_class = get_native_class("/Script/Chessboard_Roguelike.GridHealthPickupActor")
    pickup_manager_class = get_native_class("/Script/Chessboard_Roguelike.GridPickupManager")

    health_pickup_path = "/Game/Blueprints/Pickups/BP_HealthPickup"
    health_pickup_blueprint = create_blueprint(health_pickup_path, health_pickup_class)
    configure_health_pickup(health_pickup_blueprint, health_pickup_path)
    create_blueprint("/Game/Blueprints/Pickups/BP_GridPickupManager", pickup_manager_class)


main()
