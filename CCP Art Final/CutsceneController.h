#pragma once

class CutsceneController
{
public:
    bool isCameraLockActive() const {
        return phoneCutsceneActive || scriptedPanActive;
    }

    bool isPhoneCutsceneActive() const {
        return phoneCutsceneActive;
    }

    bool canTriggerPhoneCutscene() const {
        return !phoneCutsceneTriggered && !phoneCutsceneActive;
    }

    void triggerUpstairsGalleryCutscene( Engine &engineContext, DialogueSystem &dialogue ) {
        if (upstairsGalleryCutsceneTriggered) return;
        upstairsGalleryCutsceneTriggered = true;
        triggerScriptedPanCutscene( engineContext, dialogue, "I don't remember this being here...", 3.5f, 0.06f );
    }

    void triggerStudioCutscene( Engine &engineContext, DialogueSystem &dialogue ) {
        if (studioCutsceneTriggered) return;
        studioCutsceneTriggered = true;
        triggerScriptedPanCutscene( engineContext, dialogue, "What the hell happened here?", 3.5f, 0.02f );
    }

    bool hasTriggeredStudioCutscene() const {
        return studioCutsceneTriggered;
    }

    void triggerPhoneCutscene( Engine &engineContext, DialogueSystem &dialogue, const std::string &phoneAssetPath ) {
        if (!canTriggerPhoneCutscene()) return;

        phoneCutsceneActive = true;
        elapsed = 0.0f;
        phoneCutsceneTriggered = true;

        const float yaw = std::atan2( engineContext.directionY, engineContext.directionX );
        const float px = engineContext.positionX + engineContext.directionX * kPhoneForward;
        const float py = engineContext.positionY + engineContext.directionY * kPhoneForward;

        phoneModelIndex = addWorldModelInstance(
            phoneAssetPath,
            px,
            py,
            0.22f,
            rgb( 210, 210, 220 ),
            yaw + kPhoneYawOffset,
            kPhonePitch,
            kPhoneRoll,
            false,
            0.0f,
            kPhoneHeight );

        dialogue.start( {
            {"Huh, that's weird... it was just at 100%. Wait... where am I, where is everyone? Ugh my head hurts", 6.5f}
            } );
    }

    void update( Engine &engineContext, DialogueSystem &dialogue, float dt ) {
        if (scriptedPanActive)
        {
            scriptedPanElapsed += dt;

            const float ang = scriptedPanSpeed * dt;
            const float ndx = engineContext.directionX * std::cos( ang ) - engineContext.directionY * std::sin( ang );
            const float ndy = engineContext.directionX * std::sin( ang ) + engineContext.directionY * std::cos( ang );
            engineContext.directionX = ndx;
            engineContext.directionY = ndy;
            engineContext.planeX = -engineContext.directionY * FOV_TAN;
            engineContext.planeY = engineContext.directionX * FOV_TAN;
            engineContext.yaw += ang * (180.0f / 3.14159265f);

            if (engineContext.yaw > 360.0f) engineContext.yaw -= 360.0f;
            if (engineContext.yaw < 0.0f) engineContext.yaw += 360.0f;

            if (scriptedPanElapsed >= scriptedPanDuration && !dialogue.isActive())
            {
                scriptedPanActive = false;
                scriptedPanElapsed = 0.0f;
            }
        }

        if (!phoneCutsceneActive) return;

        elapsed += dt;

        if (phoneModelIndex >= 0 && phoneModelIndex < (int)g_worldModels.size())
        {
            const float yaw = std::atan2( engineContext.directionY, engineContext.directionX );
            g_worldModels[ phoneModelIndex ].x = engineContext.positionX + engineContext.directionX * kPhoneForward;
            g_worldModels[ phoneModelIndex ].y = engineContext.positionY + engineContext.directionY * kPhoneForward;
            g_worldModels[ phoneModelIndex ].yaw = yaw + kPhoneYawOffset;
            g_worldModels[ phoneModelIndex ].pitch = kPhonePitch;
            g_worldModels[ phoneModelIndex ].roll = kPhoneRoll;
            g_worldModels[ phoneModelIndex ].heightOffset = kPhoneHeight;
        }

        if (elapsed >= durationSeconds && !dialogue.isActive())
        {
            stop( engineContext );
            mesuemObjectives.setMainObjective( "Figure out what happened." );
        }
    }

    float forcedPitchOffset() const {
        if (phoneCutsceneActive) return 74.0f;
        if (scriptedPanActive) return 2.0f;
        return 0.0f;
    }

    void reset() {
        phoneCutsceneActive = false;
        phoneCutsceneTriggered = false;
        elapsed = 0.0f;
        phoneModelIndex = -1;
        scriptedPanActive = false;
        scriptedPanElapsed = 0.0f;
        upstairsGalleryCutsceneTriggered = false;
        studioCutsceneTriggered = false;
    }

private:
    void triggerScriptedPanCutscene( Engine &engineContext, DialogueSystem &dialogue, const std::string &text, float duration, float panSpeed ) {
        if (scriptedPanActive || phoneCutsceneActive) return;
        scriptedPanActive = true;
        scriptedPanElapsed = 0.0f;
        scriptedPanDuration = std::max( 0.5f, duration );
        scriptedPanSpeed = panSpeed;
        dialogue.start( { { text, scriptedPanDuration } } );
        engineContext.pitchOffset = 2.0f;
    }

    void stop( Engine &engineContext ) {
        phoneCutsceneActive = false;
        elapsed = 0.0f;
        if (phoneModelIndex >= 0 && phoneModelIndex < (int)g_worldModels.size())
        {
            g_worldModels[ phoneModelIndex ].visible = false;
        }
        phoneModelIndex = -1;
        engineContext.pitchOffset = 0.0f;
    }

    bool phoneCutsceneActive = false;
    bool phoneCutsceneTriggered = false;
    float elapsed = 0.0f;
    float durationSeconds = 6.5f;
    int phoneModelIndex = -1;
    bool scriptedPanActive = false;
    float scriptedPanElapsed = 0.0f;
    float scriptedPanDuration = 2.8f;
    float scriptedPanSpeed = 0.45f;
    bool upstairsGalleryCutsceneTriggered = false;
    bool studioCutsceneTriggered = false;
    static constexpr float kPhoneForward = 0.45f;
    static constexpr float kPhoneHeight = 0.45f;
    static constexpr float kPhoneYawOffset = 1.5707963f;
    static constexpr float kPhonePitch = -0.1f;
    static constexpr float kPhoneRoll = 0.0f;
};
