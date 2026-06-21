import unreal


def get_native_class(class_path):
    native_class = unreal.load_object(None, class_path)
    if not native_class:
        raise RuntimeError("Native class not found: {0}".format(class_path))
    return native_class


def ensure_directory(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def create_data_asset(asset_path, data_asset_class):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log("Data asset already exists: {0}".format(asset_path))
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    package_path, asset_name = asset_path.rsplit("/", 1)
    ensure_directory(package_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("DataAssetClass", data_asset_class)

    asset = asset_tools.create_asset(asset_name, package_path, data_asset_class, factory)
    if not asset:
        raise RuntimeError("Failed to create data asset: {0}".format(asset_path))

    unreal.EditorAssetLibrary.save_asset(asset_path)
    unreal.log("Created data asset: {0}".format(asset_path))
    return asset


def create_blueprint(asset_path, parent_class):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log("Blueprint already exists: {0}".format(asset_path))
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    package_path, asset_name = asset_path.rsplit("/", 1)
    ensure_directory(package_path)

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


def configure_bootstrap_blueprint(asset_path, audio_settings, initial_bgm, play_level_start):
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    generated_class = get_blueprint_generated_class(blueprint) if blueprint else None
    if not generated_class:
        unreal.log_warning("Bootstrap blueprint is not compiled yet; skipping defaults: {0}".format(asset_path))
        return

    default_object = unreal.get_default_object(generated_class)
    default_object.set_editor_property("AudioSettings", audio_settings)
    default_object.set_editor_property("InitialBGM", initial_bgm)
    default_object.set_editor_property("bPlayLevelStartSFX", play_level_start)
    unreal.EditorAssetLibrary.save_asset(asset_path)


def configure_enemy_blueprint(asset_path, audio_profile):
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not blueprint:
        unreal.log_warning("Enemy blueprint not found: {0}".format(asset_path))
        return

    if not isinstance(blueprint, unreal.Blueprint):
        unreal.log_warning("Enemy asset is not a blueprint, skipping audio profile: {0}".format(asset_path))
        return

    generated_class = get_blueprint_generated_class(blueprint)
    if not generated_class:
        unreal.log_warning("Enemy blueprint is not compiled yet; skipping audio profile: {0}".format(asset_path))
        return

    default_object = unreal.get_default_object(generated_class)
    try:
        default_object.set_editor_property("AudioProfile", audio_profile)
    except Exception as error:
        unreal.log_warning("Failed to set AudioProfile on {0}: {1}".format(asset_path, error))
        return

    unreal.EditorAssetLibrary.save_asset(asset_path)


def main():
    audio_settings_class = get_native_class("/Script/Chessboard_Roguelike.GameAudioSettingsDataAsset")
    enemy_audio_profile_class = get_native_class("/Script/Chessboard_Roguelike.EnemyAudioProfileDataAsset")
    bootstrap_actor_class = get_native_class("/Script/Chessboard_Roguelike.GameAudioBootstrapActor")

    audio_settings = create_data_asset(
        "/Game/Data/Audio/DA_GameAudioSettings",
        audio_settings_class)
    construct_melee_profile = create_data_asset(
        "/Game/Data/Audio/Enemy/DA_EnemyAudioProfile_Construct_Melee",
        enemy_audio_profile_class)
    construct_ranged_profile = create_data_asset(
        "/Game/Data/Audio/Enemy/DA_EnemyAudioProfile_Construct_Ranged",
        enemy_audio_profile_class)
    acid_melee_profile = create_data_asset(
        "/Game/Data/Audio/Enemy/DA_EnemyAudioProfile_Acid_Melee",
        enemy_audio_profile_class)
    acid_ranged_profile = create_data_asset(
        "/Game/Data/Audio/Enemy/DA_EnemyAudioProfile_Acid_Ranged",
        enemy_audio_profile_class)

    create_blueprint(
        "/Game/Blueprints/Audio/BP_GameAudioBootstrap_MainMenu",
        bootstrap_actor_class)
    create_blueprint(
        "/Game/Blueprints/Audio/BP_GameAudioBootstrap_Level",
        bootstrap_actor_class)

    configure_bootstrap_blueprint(
        "/Game/Blueprints/Audio/BP_GameAudioBootstrap_MainMenu",
        audio_settings,
        unreal.GameBGMType.MAIN_MENU,
        False)
    configure_bootstrap_blueprint(
        "/Game/Blueprints/Audio/BP_GameAudioBootstrap_Level",
        audio_settings,
        unreal.GameBGMType.LEVEL,
        True)

    configure_enemy_blueprint(
        "/Game/Blueprints/Enemies/ConstructEnemies/BP_ConstructEnemy",
        construct_melee_profile)
    configure_enemy_blueprint(
        "/Game/Blueprints/Enemies/ConstructEnemies/BP_ConstructEnemy_Melee",
        construct_melee_profile)
    configure_enemy_blueprint(
        "/Game/Blueprints/Enemies/ConstructEnemies/BP_ConstructEnemy_Ranged",
        construct_ranged_profile)
    configure_enemy_blueprint(
        "/Game/Blueprints/Enemies/AcidEnemies/BP_AcidEnemy_Melee",
        acid_melee_profile)
    configure_enemy_blueprint(
        "/Game/Blueprints/Enemies/AcidEnemies/BP_AcidEnemy_Ranged",
        acid_ranged_profile)

    unreal.EditorAssetLibrary.save_directory("/Game/Data/Audio")
    unreal.EditorAssetLibrary.save_directory("/Game/Blueprints/Audio")
    unreal.EditorAssetLibrary.save_directory("/Game/Blueprints/Enemies")


main()
