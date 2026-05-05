#pragma once

static void startCaveFinaleSequence( Engine &engineContext ) {
    if (g_caveFinaleSequenceActive || g_caveFinaleEndingStarted) return;

    g_caveFinaleSequenceActive = true;
    g_caveFinaleSequenceStage = 0;
    g_caveFinaleSequenceTimer = 0.0f;
    g_caveFinaleStageTimer = 0.0f;
    g_caveFinaleTurnStartYaw = std::atan2( engineContext.directionY, engineContext.directionX );
    g_caveFinaleBlackoutTimer = 0.0f;
    g_caveFinaleInputLocked = false;
    g_caveFinaleScreechPlayed = false;
    g_caveFinaleEndingStarted = false;
    g_endGameStateAllowRevolver = true;
    g_showHeldWeapon = true;
    if (!g_combatState.active) g_combatState.active = true;
    if (!g_combatState.hasRevolver)
    {
        g_combatState.hasRevolver = true;
        g_combatState.loadedAmmo = 6;
        g_combatState.reserveAmmo = 0;
    }
}

static void updateCaveEndingTerminalTypewriter( float dt ) {
    if (g_caveEndingPostLinePause > 0.0f)
    {
        g_caveEndingPostLinePause = std::max( 0.0f, g_caveEndingPostLinePause - dt );
        return;
    }

    if (g_caveEndingTypingLine.empty())
    {
        if (g_caveEndingTypeQueue.empty())
        {
            if (!g_caveEndingCreditsActive)
            {
                g_caveEndingCreditsActive = true;
                g_caveEndingCreditsTimer = 0.0f;
                g_caveEndingCreditsScroll = float( RENDER_H ) + 80.0f;
            }
            return;
        }

        g_caveEndingTypingLine = g_caveEndingTypeQueue.front();
        g_caveEndingTypeQueue.pop_front();
        g_caveEndingTypingChars = 0;
        g_caveEndingTypingAccumulator = 0.0f;
    }

    g_caveEndingTypingAccumulator += dt * 56.0f;
    auto charCost = []( char c ) -> float {
        if (c == ' ') return 0.45f;
        if (c == '.' || c == ',' || c == ':' || c == ';') return 1.8f;
        if (c == '?' || c == '!') return 2.2f;
        if (c == '/' || c == '\\' || c == '-' || c == '_' || c == '=') return 1.2f;
        return 1.0f;
    };

    while (g_caveEndingTypingChars < g_caveEndingTypingLine.size())
    {
        const float cost = charCost( g_caveEndingTypingLine[ g_caveEndingTypingChars ] );
        if (g_caveEndingTypingAccumulator < cost) break;
        ++g_caveEndingTypingChars;
        g_caveEndingTypingAccumulator -= cost;
    }

    if (g_caveEndingTypingChars >= g_caveEndingTypingLine.size())
    {
        g_caveEndingTerminalLog.push_back( g_caveEndingTypingLine );
        g_caveEndingTypingLine.clear();
        g_caveEndingTypingChars = 0;
        g_caveEndingTypingAccumulator = 0.0f;
        g_caveEndingPostLinePause = 0.16f;
    }
}

static void updateCaveFinaleSequence( Engine &engineContext, GameState &currentState, float dt ) {
    if (!g_caveFinaleSequenceActive) return;

    g_caveFinaleSequenceTimer += dt;
    g_caveFinaleStageTimer += dt;

    const float currentYaw = std::atan2( engineContext.directionY, engineContext.directionX );
    const auto setYaw = [&]( float yaw ) {
        engineContext.directionX = std::cos( yaw );
        engineContext.directionY = std::sin( yaw );
        engineContext.planeX = -engineContext.directionY * FOV_TAN;
        engineContext.planeY = engineContext.directionX * FOV_TAN;
        engineContext.yaw = yaw * (180.0f / 3.14159265f);
        if (engineContext.yaw > 360.0f) engineContext.yaw -= 360.0f;
        if (engineContext.yaw < 0.0f) engineContext.yaw += 360.0f;
    };

    if (g_caveFinaleSequenceStage == 0)
    {
        g_caveFinaleInputLocked = false;
        if (!g_caveFinaleScreechPlayed)
        {
            playCaveMonsterScreech( g_currentLevelFolder, kCaveFinaleTurnTriggerX, kCaveFinaleTurnTriggerY );
            showAccessPopup( "RUN", 2200 );
            g_caveFinaleScreechPlayed = true;
        }

        g_cutsceneController.updateTurnHeadShake( dt, true, 1.15f );
        engineContext.pitchOffset = std::sin( g_caveFinaleSequenceTimer * 25.0f ) * 1.8f;

        const float distX = engineContext.positionX - kCaveFinaleTurnTriggerX;
        const float distY = engineContext.positionY - kCaveFinaleTurnTriggerY;
        if ((distX * distX + distY * distY) <= (kCaveFinaleTurnTriggerRadius * kCaveFinaleTurnTriggerRadius) && g_caveFinaleSequenceTimer >= 1.25f)
        {
            g_caveFinaleSequenceStage = 1;
            g_caveFinaleStageTimer = 0.0f;
            g_caveFinaleTurnStartYaw = currentYaw;
            g_caveFinaleInputLocked = true;
        }
    }
    else if (g_caveFinaleSequenceStage == 1)
    {
        g_caveFinaleInputLocked = true;
        const float turnDuration = 1.8f;
        const float p = std::clamp( g_caveFinaleStageTimer / turnDuration, 0.0f, 1.0f );
        const float ease = p * p * (3.0f - 2.0f * p);
        const float turnTarget = g_caveFinaleTurnStartYaw + 3.14159265f;
        const float delta = std::atan2( std::sin( turnTarget - g_caveFinaleTurnStartYaw ), std::cos( turnTarget - g_caveFinaleTurnStartYaw ) );
        setYaw( g_caveFinaleTurnStartYaw + delta * ease );
        engineContext.pitchOffset = std::sin( g_caveFinaleStageTimer * 31.0f ) * 1.45f;
        g_cutsceneController.updateTurnHeadShake( dt, true, 1.25f );

        if (g_caveFinaleStageTimer >= 10.0f)
        {
            g_caveFinaleSequenceStage = 2;
            g_caveFinaleStageTimer = 0.0f;
            g_caveFinaleBlackoutTimer = 0.0f;
        }
    }
    else if (g_caveFinaleSequenceStage == 2)
    {
        g_caveFinaleInputLocked = true;
        g_caveFinaleBlackoutTimer += dt;
        engineContext.pitchOffset = 0.0f;
        g_cutsceneController.updateTurnHeadShake( dt, false, 0.0f );

        if (g_caveFinaleBlackoutTimer >= 1.15f)
        {
            g_caveFinaleSequenceActive = false;
            g_caveFinaleInputLocked = false;
            g_caveFinaleSequenceStage = 0;
            g_caveFinaleSequenceTimer = 0.0f;
            g_caveFinaleStageTimer = 0.0f;
            g_caveFinaleBlackoutTimer = 0.0f;
            g_caveFinaleScreechPlayed = false;
            currentState = STATE_ENDING;
            startCaveEndingTerminalSequence();
        }
    }
}

static void updateCaveEndingSequence( Engine &engineContext, float dt ) {
    if (g_caveFinaleSequenceActive || !g_caveFinaleEndingStarted) return;

    updateCaveEndingTerminalTypewriter( dt );
    if (g_caveEndingCreditsActive)
    {
        g_caveEndingCreditsTimer += dt;
        g_caveEndingCreditsScroll -= dt * 24.0f;
    }
    else
    {
        engineContext.pitchOffset = 0.0f;
    }
}

static void renderCaveFinaleOverlay( Engine &engineContext ) {
    if (!g_caveFinaleSequenceActive) return;

    if (g_caveFinaleSequenceStage == 0)
    {
        drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), 0.28f );
        drawTextBox( engineContext, (RENDER_W / 2) - 124, 28, 248, 74, rgb( 10, 10, 14 ), rgb( 210, 40, 40 ) );
        drawString16x16( engineContext, (RENDER_W / 2) - 38, 52, "RUN", rgb( 250, 80, 80 ), 80, 1, 1, false );
    }
    else if (g_caveFinaleSequenceStage == 1)
    {
        const float p = std::clamp( g_caveFinaleStageTimer / 10.0f, 0.0f, 1.0f );
        drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), std::clamp( 0.10f + p * 0.08f, 0.10f, 0.20f ) );
    }
    else if (g_caveFinaleSequenceStage == 2)
    {
        const float p = std::clamp( g_caveFinaleBlackoutTimer / 1.15f, 0.0f, 1.0f );
        drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), std::clamp( 0.45f + p * 0.55f, 0.45f, 1.0f ) );
    }
}

static void renderCaveEndingScreen( Engine &engineContext ) {
    drawTextBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );
    const Uint32 terminalInk = rgb( 115, 230, 145 );
    const Uint32 dimInk = rgb( 70, 150, 95 );
    const int boxX = 34;
    const int boxY = 26;
    const int boxW = RENDER_W - 68;
    const int boxH = RENDER_H - 52;
    drawTranslucentBox( engineContext, boxX, boxY, boxW, boxH, rgb( 0, 0, 0 ), 0.78f );
    drawStringTinyScaled( engineContext, boxX + 12, boxY + 10, "[ PROJECT TERMINAL / FINAL LOG ]", terminalInk, 1, 1, 1, false );

    const int logX = boxX + 12;
    const int logY = boxY + 28;
    const int rowStep = 12;
    const int visibleLines = std::max( 1, (boxH - 110) / rowStep );
    int start = 0;
    if ((int)g_caveEndingTerminalLog.size() > visibleLines) start = (int)g_caveEndingTerminalLog.size() - visibleLines;

    int y = logY;
    for (int i = start; i < (int)g_caveEndingTerminalLog.size(); ++i)
    {
        drawStringTinyScaled( engineContext, logX, y, g_caveEndingTerminalLog[ i ], terminalInk, 1, 1, 1, false );
        y += rowStep;
    }

    if (!g_caveEndingTypingLine.empty())
    {
        std::string typed = g_caveEndingTypingLine.substr( 0, std::min( g_caveEndingTypingChars, g_caveEndingTypingLine.size() ) );
        drawStringTinyScaled( engineContext, logX, y, typed, dimInk, 1, 1, 1, false );
        y += rowStep;
    }

    if (g_caveEndingCreditsActive)
    {
        const std::vector<std::string> credits = {
            "",
            "CREDITS",
            "Lines of code: ~18,000",
            "Hours: ~200",
            "Total Play Time: ~20 Minutes",
            "Accelerated By Google Gemini and OpenAI ChatGPT",
            ""
        };

        int cy = std::max( y + 24, int( g_caveEndingCreditsScroll ) );
        for (const auto &line : credits)
        {
            drawString16x16( engineContext, boxX + 18, cy, normalizeCaveEndingTerminalText( line ), terminalInk, boxW - 36, 1, 1, false );
            cy += 22;
        }
    }

    drawStringTinyScaled( engineContext, boxX + 12, boxY + boxH - 18, "ESC TO RETURN TO MENU   R TO RESTART", dimInk, 1, 1, 1, false );
}
