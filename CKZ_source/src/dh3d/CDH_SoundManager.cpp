// CDH_SoundManager.cpp - sounds and music of the game.
#include "CDH_SoundManager.h"
#include <string.h>
#include "CApplet.h"
#include "CApp.h"
#include "ICMediaPlayer.h"
#include "ICResourceManager.h"
#include "CResource.h"
#include "CRandGen.h"
#include "CVector.h"
#include "App.h"
#include "AppSettings.h"
#include "CDH_ResourceManager.h"

using namespace com::glu::platform;

/** Seconds per Update() millisecond. */
static const float MS_TO_SECONDS = 0.001f;
/** Music level while a track with fadeMusic plays, normal level and fade speed. */
static const unsigned long TRACK_MUSIC_LEVEL = 20;
static const unsigned long FULL_MUSIC_LEVEL = 100;
static const unsigned long MUSIC_FADE_SPEED = 100;

const char* CDH_SoundManager::SOUND_TO_RESOURCE_REFLECTION[SOUND_COUNT] = {
    // b108: every entry read back from the original table (__DATA,__data 0x5000d0); the ref dump showed
    // some string pointers as "" and truncated the rest, which left 74 entries empty (map crash).
    "IDM_GRENADE_EXPLOSION", "IDM_BARREL_EXPLOSION", "IDM_WEAPON_CHANGE", "IDM_FIRE_SHOTGUN",
    "IDM_FIRE_REVOLVER", "IDM_FIRE_ASR", "IDM_FIRE_SUBMACHINE", "IDM_FIRE_SNIPER",
    "IDM_FIRE_REVOLUTION", "IDM_FIRE_ANTIMATERIEL", "IDM_RELOAD_ANTIMATERIEL", "IDM_RELOAD_SUBMACHINE",
    "IDM_RELOAD_SNIPER", "IDM_RELOAD_M79", "IDM_SHOT_HEALING_GUN", "IDM_RELOAD_HEALING_GUN",
    "IDM_START_RELOAD_SHOTGUN", "IDM_RELOAD_SHOTGUN_1", "IDM_RELOAD_SHOTGUN_2", "IDM_END_RELOAD_SHOTGUN",
    "IDM_HELI_LOOP_01", "IDM_RELOAD_REVOLVER", "IDM_RELOAD_ASR", "IDM_HEADSHOT",
    "IDM_SKILLSHOT", "IDM_DISINTEGRATE_01", "IDM_DISINTEGRATE_02", "IDM_MENU_WINDOW_BUTTON",
    "IDM_ACHIEVEMENT", "IDM_LEVEL_UP", "IDM_MISSION_START", "IDM_MISSION_COMPLETED",
    "IDM_MISSION_FAILED", "IDM_WAVE_START", "IDM_MENU_SHOP_ITEM_TAPPED", "IDM_MENU_SHOP_ITEM_BOUGHT",
    "IDM_MENU_SHOP_ITEM_EQUIPPED", "IDM_MENU_SHOP_ITEM_USED", "IDM_MENU_SHOP_NA_MONEY", "IDM_ZOMBIE_WALK_01",
    "IDM_ZOMBIE_ATTACK_01", "IDM_ZOMBIE_HIT_SNARL_01", "IDM_ZOMBIE_DEATH_01", "IDM_ZOMBIE_WALK_02",
    "IDM_ZOMBIE_ATTACK_02", "IDM_ZOMBIE_HIT_SNARL_02", "IDM_ZOMBIE_DEATH_02", "IDM_ZOMBIE_WALK_03",
    "IDM_ZOMBIE_ATTACK_03", "IDM_ZOMBIE_HIT_SNARL_03", "IDM_ZOMBIE_DEATH_03", "IDM_CIVILIAN_MALE_WOUND_01",
    "IDM_CIVILIAN_MALE_WOUND_02", "IDM_CIVILIAN_MALE_DEATH_01", "IDM_CIVILIAN_MALE_DEATH_02", "IDM_CIVILIAN_MALE_DEATH_03",
    "IDM_CIVILIAN_FEMALE_WOUND_01", "IDM_CIVILIAN_FEMALE_WOUND_02", "IDM_CIVILIAN_FEMALE_WOUND_03", "IDM_CIVILIAN_FEMALE_WOUND_04",
    "IDM_CIVILIAN_FEMALE_DEATH_01", "IDM_CIVILIAN_FEMALE_DEATH_02", "IDM_GIRL_ATTACK_01", "IDM_GIRL_ATTACK_02",
    "IDM_GIRL_ATTACK_03", "IDM_GIRL_ATTACK_04", "IDM_GIRL_DEATH_01", "IDM_GIRL_DEATH_02",
    "IDM_GIRL_DEATH_03", "IDM_GIRL_DEATH_04", "IDM_GIRL_WOUND_01", "IDM_GIRL_WOUND_02",
    "IDM_GIRL_WOUND_03", "IDM_GIRL_WOUND_04", "IDM_GIRL_WOUND_05", "IDM_SHOOTER_SKILLSHOT_01",
    "IDM_SHOOTER_SKILLSHOT_02", "IDM_SHOOTER_SKILLSHOT_03", "IDM_SHOOTER_SKILLSHOT_04", "IDM_SHOOTER_SKILLSHOT_05",
    "IDM_SHOOTER_SKILLSHOT_06", "IDM_SHOOTER_SKILLSHOT_07", "IDM_SHOOTER_SKILLSHOT_08", "IDM_SHOOTER_SKILLSHOT_09",
    "IDM_SHOOTER_SKILLSHOT_10", "IDM_SHOOTER_SKILLSHOT_11", "IDM_SHOOTER_WAVE_01", "IDM_SHOOTER_WAVE_02",
    "IDM_SHOOTER_WAVE_03", "IDM_SHOOTER_WAVE_04", "IDM_FIRE_EMPTY", "IDM_SCOPE_ON",
    "IDM_SCOPE_OFF", "IDM_MENU_PURCHASE_COMPLETE", "IDM_MENU_WINDOW_OPEN_CLOSE", "IDM_ENEMY_ASSAULT",
    "IDM_ENEMY_SUBMACHINE", "IDM_ENEMY_SHOTGUN", "IDM_ENEMY_SNIPER", "IDM_ENEMY_MACHINE",
    "IDM_ENEMY_HANDGUN", "IDM_MINIGUN_FIRE", "IDM_MINIGUN_ROTATE_START", "IDM_MINIGUN_ROTATE_LOOP",
    "IDM_RELOAD_AUTO_SHOTGUN", "IDM_RELOAD_HEAVY_PISTOL", "IDM_RELOAD_MACHINEGUN", "IDM_RELOAD_MINIGUN",
    "IDM_RELOAD_MGL", "IDM_RELOAD_UZI", "IDM_SWITCH_ANTIMATERIEL", "IDM_SWITCH_ASR",
    "IDM_SWITCH_AUTO_SHOTGUN", "IDM_SWITCH_HEAVY_PISTOL", "IDM_SWITCH_M79", "IDM_SWITCH_MACHINEGUN",
    "IDM_SWITCH_HEALING_GUN", "IDM_SWITCH_MINIGUN", "IDM_SWITCH_MGL", "IDM_SWITCH_REVOLVER",
    "IDM_SWITCH_SHOTGUN", "IDM_SWITCH_SNIPER", "IDM_SWITCH_UZI", "IDM_GRENADE_THROW",
    "IDM_BEACON_GRENADE_BEEP", "IDM_POWERUP_DOUBLE_DAMAGE", "IDM_POWERUP_FAST_SHOOTING", "IDM_POWERUP_SLOW_MOTION",
    "IDM_ZOMBIE_BOSS_HIT_01", "IDM_ZOMBIE_BOSS_HIT_02", "IDM_ZOMBIE_BOSS_HIT_03", "IDM_ZOMBIE_BOSS_HIT_04",
    "IDM_ZOMBIE_BOSS_DEATH", "IDM_ZOMBIE_BOSS_PICKUP", "IDM_ZOMBIE_BOSS_SPAWN", "IDM_ZOMBIE_BOSS_THROW",
    "IDM_ZOMBIE_BOSS_RAGE", "IDM_ZOMBIE_BOSS_FOOTSTEPS_LEFT", "IDM_ZOMBIE_BOSS_FOOTSTEPS_RIGHT", "IDM_SILENCED_SMG_FIRE",
    "IDM_SWITCH_RPG7", "IDM_RELOAD_RPG7", "IDM_FIRE_RPG7", "IDM_EXPLOSION_RPG7",
    "IDM_ENGINE_RPG7", "IDM_SWITCH_RL4", "IDM_RELOAD_RL4", "IDM_FIRE_RL4",
    "IDM_EXPLOSION_RL4", "IDM_ENGINE_RL4", "IDM_ZOMBIE_MUTANT_ATTACK", "IDM_ZOMBIE_MUTANT_HIT",
    "IDM_ZOMBIE_MUTANT_DEATH", "IDM_ROTTEN_ZOMBIE_ATTACK", "IDM_ROTTEN_ZOMBIE_HIT", "IDM_ROTTEN_ZOMBIE_DEATH",
    "IDM_MENU_MUSIC", "IDM_AMBIENT_1", "IDM_AMBIENT_2", "IDM_AMBIENT_3",
};

/** The resource manager of the application. */
static inline systems::ICResourceManager* Resources()
{
    return (systems::ICResourceManager*)CApplet::m_pApp->m_resourceManager;
}

int CDH_SoundManager::GetSoundFromResourceID(const char* resourceId)
{
    for (int sound = 0; sound < SOUND_COUNT; sound++) {
        if (strcmp(SOUND_TO_RESOURCE_REFLECTION[sound], resourceId) == 0) {
            return sound;
        }
    }
    return SOUND_COUNT;
}

CDH_SoundManager::CDH_SoundManager()
    : m_musicLevel(FULL_MUSIC_LEVEL),
      m_targetMusicLevel(FULL_MUSIC_LEVEL),
      m_musicFadeSpeed(0)
{
    m_classId = ClassId;
    m_sounds = new void*[SOUND_COUNT];
    for (int sound = 0; sound < SOUND_COUNT; sound++) {
        m_eventIds[sound] = (unsigned int)-1;
        m_sounds[sound] = NULL;
    }
    m_activated = false;
}

CDH_SoundManager::~CDH_SoundManager()
{
    StopMusic();
    StopSounds();
    if (m_sounds != NULL) {
        delete[] m_sounds;
        m_sounds = NULL;
    }
}

void CDH_SoundManager::initResource(eSound sound)
{
    if (m_sounds[sound] == NULL) {
        systems::CResource* resource = NULL;
        Resources()->CreateResource(SOUND_TO_RESOURCE_REFLECTION[sound], &resource);
        m_sounds[sound] = resource->GetData();
    }
}

void CDH_SoundManager::StopAllSoundsAndRelease()
{
    StopMusic();
    StopSounds();
    releaseResources();
}

void CDH_SoundManager::releaseResources()
{
    for (int sound = 0; sound < SOUND_COUNT; sound++) {
        if (m_sounds[sound] != NULL) {
            Resources()->ReleaseResource(SOUND_TO_RESOURCE_REFLECTION[sound]);
            m_sounds[sound] = NULL;
        }
    }
}

core::CSharedPtr<TrackHandler> CDH_SoundManager::PlayTrack(const char* name, bool fadeMusic)
{
    if (!((App*)WindowApp::m_instance)->m_settings->IsSoundEnabled()) {
        return core::CSharedPtr<TrackHandler>((TrackHandler*)NULL);
    }
    TrackHandler* track = new TrackHandler(components::ICMediaPlayer::GetInstance(), name, fadeMusic);
    core::CSharedPtr<TrackHandler> handler(track);
    m_tracks.addElement(handler);
    return handler;
}

void CDH_SoundManager::Update(unsigned long elapsedMs)
{
    UpdateMediaPlayerSettings();
    float elapsed = elapsedMs * MS_TO_SECONDS;
    int deferredCount = m_deferred.size();
    for (int index = 0; index < deferredCount; index++) {
        m_deferred[index].m_delay -= elapsed;
    }
    if (deferredCount != 0 && m_deferred[0].m_delay <= 0.0f) {
        Play(m_deferred[0].m_sound, m_deferred[0].m_exclusive, false);
        m_deferred.removeElementAt(0);
    }
    if (m_musicFadeSpeed != 0) {
        unsigned long level = m_musicLevel;
        unsigned long target = m_targetMusicLevel;
        if (level != target) {
            unsigned long step = elapsedMs * m_musicFadeSpeed / 1000;
            unsigned long change = step;
            bool done = false;
            if (target <= level) {
                if (level <= target) {
                    m_musicFadeSpeed = 0;
                    done = true;
                } else {
                    change = -step;
                }
            }
            if (!done) {
                if (step < target - level) {
                    target = level + change;
                }
                SetMusicLevel(target);
            }
        }
    }
    int trackCount = m_tracks.size();
    for (int index = 0; index < trackCount; index++) {
        TrackHandler* track = m_tracks[index].Get();
        if (track->m_player->IsPlaying(track->m_eventId)) {
            m_tracks[index]->update(elapsedMs);
        }
        trackCount = m_tracks.size();
    }
    // drop the finished tracks (the last one moves into the hole)
    for (int index = 0; index < trackCount; index++) {
        TrackHandler* track = m_tracks[index].Get();
        if (!track->m_player->IsPlaying(track->m_eventId)) {
            trackCount--;
            m_tracks[index] = core::CSharedPtr<TrackHandler>();
            m_tracks[index] = m_tracks[trackCount];
        }
    }
    m_tracks.setSize(trackCount);
}

void CDH_SoundManager::UpdateMediaPlayerSettings()
{
    AppSettings* settings = ((App*)WindowApp::m_instance)->m_settings;
    bool soundEnabled = false;
    if (m_activated) {
        soundEnabled = true;
        if (!settings->IsSoundEnabled()) {
            soundEnabled = settings->IsMusicEnabled();
        }
    }
    bool vibrationEnabled = false;
    if (m_activated) {
        vibrationEnabled = settings->IsVibrationEnabled();
    }
    int volume = settings->Volume();
    components::ICMediaPlayer* player = components::ICMediaPlayer::GetInstance();
    if (player->GetSoundEnabled() != soundEnabled) {
        player->SetSoundEnabled(soundEnabled);
    }
    if (player->GetVibrationEnabled() != vibrationEnabled) {
        player->SetVibrationEnabled(vibrationEnabled);
    }
    if (player->GetVolume() != volume) {
        player->SetVolume(volume);
    }
}

void CDH_SoundManager::Play(eSound sound, bool exclusive, bool loop)
{
    bool isMusic = (unsigned int)(sound - SOUND_MUSIC_FIRST) < MUSIC_COUNT;
    if (isMusic && ((App*)WindowApp::m_instance)->m_settings->IsMusicEnabled() != true) {
        return;
    }
    if (sound < SOUND_MUSIC_FIRST && ((App*)WindowApp::m_instance)->m_settings->IsSoundEnabled() != true) {
        return;
    }
    initResource(sound);
    bool shouldLoop = loop || isMusic;
    if (exclusive == true && IsPlaying(sound)) {
        return;
    }
    components::ICMediaPlayer* player = components::ICMediaPlayer::GetInstance();
    m_eventIds[sound] = player->Play((components::CMedia*)m_sounds[sound], shouldLoop, 0);
}

void CDH_SoundManager::SetMusicLevel(unsigned long level)
{
    unsigned long clamped = level;
    if (clamped > FULL_MUSIC_LEVEL) {
        clamped = FULL_MUSIC_LEVEL;
    }
    m_musicLevel = clamped;
    for (int music = 0; music < MUSIC_COUNT; music++) {
        if (IsPlaying((eSound)(SOUND_MUSIC_FIRST + music))) {
            components::ICMediaPlayer::GetInstance()->SetProperty(m_eventIds[SOUND_MUSIC_FIRST + music],
                                                                  (components::ICMediaPlayer::SoundProperty)0,
                                                                  level);
        }
    }
}

void TrackHandler::update(int elapsedMs)
{
    if (m_state != STATE_FADING_OUT) {
        return;
    }
    if (m_fadeLeft < (unsigned int)elapsedMs) {
        m_fadeLeft = 0;
        m_player->SetProperty(m_eventId, (components::ICMediaPlayer::SoundProperty)0, 0);
        stop();
        return;
    }
    m_fadeLeft -= elapsedMs;
    int volume = m_fadeLeft * 100 / m_fadeDuration;
    int clamped = 0;
    if (volume >= 0) {
        if (volume > 100) {
            volume = 100;
        }
        clamped = volume;
    }
    m_player->SetProperty(m_eventId, (components::ICMediaPlayer::SoundProperty)0, clamped);
}

unsigned char CDH_SoundManager::IsPlaying(eSound sound)
{
    return components::ICMediaPlayer::GetInstance()->IsPlaying(m_eventIds[sound]);
}

float CDH_SoundManager::GetSoundDuration(eSound sound, unsigned int bytesPerSample, unsigned int sampleRate)
{
    unsigned int* media = (unsigned int*)m_sounds[sound];
    if (media == NULL) {
        initResource(sound);
        media = (unsigned int*)m_sounds[sound];
    }
    // the media object keeps its data size at 0x0c
    return (float)media[3] / (float)(sampleRate * bytesPerSample);
}

void CDH_SoundManager::StopMusic()
{
    for (int music = 0; music < MUSIC_COUNT; music++) {
        if (IsPlaying((eSound)(SOUND_MUSIC_FIRST + music))) {
            components::ICMediaPlayer::GetInstance()->Stop(m_eventIds[SOUND_MUSIC_FIRST + music]);
        }
    }
}

void CDH_SoundManager::StopSounds()
{
    for (int sound = 0; sound < SOUND_MUSIC_FIRST; sound++) {
        if (IsPlaying((eSound)sound)) {
            components::ICMediaPlayer::GetInstance()->Stop(m_eventIds[sound]);
        }
    }
}

void CDH_SoundManager::Stop(eSound sound)
{
    components::ICMediaPlayer::GetInstance()->Stop(m_eventIds[sound]);
}

void CDH_SoundManager::Pause(eSound sound)
{
    components::ICMediaPlayer::GetInstance()->Pause(m_eventIds[sound]);
}

void CDH_SoundManager::Resume(eSound sound)
{
    components::ICMediaPlayer::GetInstance()->Resume(m_eventIds[sound]);
}

int CDH_SoundManager::GetRandomSound(const char* keySet)
{
    CDH_ResourceManager* resources = ((App*)WindowApp::m_instance)->m_resourceManager;
    CDH_KeySet* keys = (CDH_KeySet*)resources->getResourceData(keySet, false);
    core::CRandGen* random = core::CRandGen::GetInstance();
    int index = random->GetRandRange(0, keys->m_count - 1);
    int sound = GetSoundFromResourceID(keys->GetKey(index));
    ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(keySet);
    return sound;
}

int CDH_SoundManager::PlayRandomSoundWithDelay(const char* keySet, float delay, bool exclusive)
{
    // Earlier recovery: result unused (the original draws twice).
    // 0x162c46 saves the first id in r8; 0x162c96 returns it on both exit paths.
    int firstSound = GetRandomSound(keySet);
    SDeferredPlayback playback;
    playback.m_sound = (eSound)GetRandomSound(keySet);
    playback.m_delay = delay;
    playback.m_exclusive = exclusive;
    int count = m_deferred.size();
    for (int index = 0; index < count; index++) {
        if (delay < m_deferred[index].m_delay) {
            m_deferred.insertElementAt(playback, index);
            if (m_deferred.size() > count) {
                return firstSound;
            }
            break;
        }
    }
    m_deferred.addElement(playback);
    return firstSound;
}

void CDH_SoundManager::PrecacheSound(eSound sound)
{
    initResource(sound);
}

void CDH_SoundManager::PrecacheKeysetSounds(const char* keySet)
{
    CDH_KeySet* keys = (CDH_KeySet*)((App*)WindowApp::m_instance)->m_resourceManager->getResourceData(keySet, false);
    unsigned int count = keys->m_count;
    for (unsigned int index = 0; index < count; index++) {
        initResource((eSound)GetSoundFromResourceID(keys->GetKey(index)));
    }
    ((App*)WindowApp::m_instance)->m_resourceManager->releaseResource(keySet);
}

void CDH_SoundManager::Pause()
{
    components::ICMediaPlayer* player = components::ICMediaPlayer::GetInstance();
    player->Pause(0);
    player->PauseVibrate(0);
}

void CDH_SoundManager::Resume()
{
    components::ICMediaPlayer* player = components::ICMediaPlayer::GetInstance();
    player->Resume(0);
    player->ResumeVibrate(0);
    // apply the settings once with everything off so the media player really restarts
    AppSettings* settings = ((App*)WindowApp::m_instance)->m_settings;
    bool soundEnabled = settings->IsSoundEnabled();
    bool musicEnabled = settings->IsMusicEnabled();
    bool vibrationEnabled = settings->IsVibrationEnabled();
    int volume = settings->Volume();
    settings->SetSoundEnabled(false);
    settings->SetMusicEnabled(false);
    settings->SetVibrationEnabled(false);
    settings->SetVolume(0);
    Update(0);
    settings->SetSoundEnabled(soundEnabled);
    settings->SetMusicEnabled(musicEnabled);
    settings->SetVibrationEnabled(vibrationEnabled);
    settings->SetVolume(volume);
    Update(0);
}

void CDH_SoundManager::SetActivated(bool activated)
{
    m_activated = activated;
    UpdateMediaPlayerSettings();
}

TrackHandler::TrackHandler(components::ICMediaPlayer* player, const char* name, bool fadeMusic)
{
    m_player = player;
    m_resourceHandle = 0;
    m_fadeDuration = 0;
    m_fadeLeft = 0;
    systems::CResource* resource = NULL;
    Resources()->CreateResource(name, &resource);
    m_state = STATE_STOPPED;
    m_eventId = 0;
    if (resource != NULL) {
        void* media = resource->GetData();
        m_resourceHandle = resource->m_handle;
        m_eventId = player->Play((components::CMedia*)media, 0, 0xff);
        m_state = (m_eventId != 0);
        if (fadeMusic) {
            CDH_SoundManager* sounds = ((App*)WindowApp::m_instance)->m_soundManager;
            sounds->m_targetMusicLevel = TRACK_MUSIC_LEVEL;
            sounds->m_musicFadeSpeed = MUSIC_FADE_SPEED;
        }
    }
}

void TrackHandler::stop()
{
    if (m_state == STATE_STOPPED) {
        return;
    }
    m_player->Stop(m_eventId);
    if (m_resourceHandle != 0) {
        Resources()->ReleaseResource(m_resourceHandle);
        m_resourceHandle = 0;
    }
    m_state = STATE_STOPPED;
    m_eventId = 0;
    CDH_SoundManager* sounds = ((App*)WindowApp::m_instance)->m_soundManager;
    sounds->m_targetMusicLevel = FULL_MUSIC_LEVEL;
    sounds->m_musicFadeSpeed = MUSIC_FADE_SPEED;
}

void TrackHandler::fadeOut(unsigned long durationMs)
{
    if (m_state == STATE_PLAYING) {
        m_state = STATE_FADING_OUT;
        m_fadeDuration = durationMs;
        m_fadeLeft = durationMs;
    }
}
