#include "global.h"
#include "main.h"
#include "task.h"
#include "m4a.h"
#include "bg.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "palette.h"
#include "scanline_effect.h"
#include "dma3.h"
#include "sprite.h"
#include "window.h"
#include "text.h"
#include "strings.h"
#include "menu.h"
#include "string_util.h"
#include "constants/flags.h"
#include "constants/rgb.h"

static const u8 sAchievementAtlasTileset[] = INCBIN_U8("graphics/achievement_atlas/achievement_atlas.4bpp");
static const u16 sAchievementAtlasTilemap[] = INCBIN_U16("graphics/achievement_atlas/achievement_atlas.bin");
static const u8 sAchievementAtlasPalette[] = INCBIN_U8("graphics/achievement_atlas/achievement_atlas.gbapal");

static const u8 sAtlasCursorTiles[] = INCBIN_U8("graphics/achievement_atlas/cursor.4bpp");
static const u16 sAtlasCursorPalette[] = INCBIN_U16("graphics/achievement_atlas/cursor.gbapal");

static const u8 sDescriptionNotAvailable[] = _("Desctiption not available.");

static const u8 sWholeNewWorldAchLabel[] = _("A Whole New World");
static const u8 sWholeNewWorldAchDesc[] = _("Welcome to Emerald Enhanced.\nExplore to your heart's content!");

static const u8 sEnhancedBattleAchLabel[] = _("Enhanced Battle");
static const u8 sEnhancedBattleAchDesc[] = _("You have awakened as a battle master!");

static const u8 sNewRegionWhoDisAchLabel[] = _("New Region Who Dis");
static const u8 sNewRegionWhoDisAchDesc[] = _("You made a friend!\nEverything is better with friends.");

static const u8 sAdventureTimeAchLabel[] = _("Adventure Time");
static const u8 sAdventureTimeAchDesc[] = _("You started your first quest.\nGood Luck!");

static const u8 sTrueLoveAchLabel[] = _("Love at First Sight");
static const u8 sTrueLoveAchDesc[] = _("You have entered a relationship.\nNow you wonder what you would\ndo without them.");

static const u8 sSilentStrongTypeAchLabel[] = _("Silent, Strong type");
static const u8 sSilentStrongTypeAchDesc[] = _("The quiet ones usually have the\nmost to say!\n {COLOR LIGHT_GREEN}{SHADOW GREEN}(Romanced Lana)");

static const u8 sChildhoodFriendAchLabel[] = _("A Childhood Friend");
static const u8 sChildhoodFriendAchDesc[] = _("{RIVAL} would follow you off\nof a cliff.\n {COLOR LIGHT_GREEN}{SHADOW GREEN}(Romanced {RIVAL})");

static const u8 sNerdLoveAchLabel[] = _("Nerd Love");
static const u8 sNerdLoveAchDesc[] = _("You feel her rubbing off on you.\n {COLOR LIGHT_GREEN}{SHADOW GREEN}(Romanced Lanette)");

static const u8 sFWBAchLabel[] = _("Friends with Benefits");
static const u8 sFWBAchDesc[] = _("You have a secret weapon!\nYour own, personal nurse.\n {COLOR LIGHT_GREEN}{SHADOW GREEN}(Romanced Joy)");

static const u8 sWetnWildAchLabel[] = _("Wet and Wild");
static const u8 sWetnWildAchDesc[] = _("You found a girl who loves to\nget wet.\n {COLOR LIGHT_GREEN}{SHADOW GREEN}(Romanced Shelly)");

static const u8 sFieryPassionAchLabel[] = _("Fiery Passion");
static const u8 sFieryPassionAchDesc[] = _("She's blazing hot!\n {COLOR LIGHT_GREEN}{SHADOW GREEN}(Romanced Courtney)");

static const u8 sTrueEndingAchLabel[] = _("The True Ending");
static const u8 sTrueEndingAchDesc[] = _("You're quite the lady killer, eh?\n {COLOR LIGHT_GREEN}{SHADOW GREEN}(Got the Harem ending)");

void DecideActionFromInput(u32 * action);
void AtlasCursorSpriteCB(struct Sprite *);

enum
{
    CATEGORY_EXPLORATION,  //dark_grey/light_grey. also for misc achievements
    CATEGORY_BATTLE,       //light_red/red
    CATEGORY_COMPANIONS,   //light_blue/blue
    CATEGORY_QUESTING      //light_green/green
};

struct AtlasAchPointData
{
    u8 x;
    u8 y;
    u8 category;
    u8 flagId;
    const u8 * nameString;
    const u8 * descString;
};

static const u8 sTextColors[][3] = {
    [CATEGORY_EXPLORATION] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GREY, TEXT_COLOR_LIGHT_GREY},
    [CATEGORY_BATTLE] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_RED, TEXT_COLOR_RED},
    [CATEGORY_COMPANIONS] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_BLUE, TEXT_COLOR_BLUE},
    [CATEGORY_QUESTING] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_LIGHT_GREEN, TEXT_COLOR_GREEN},
};

static const struct AtlasAchPointData sTestAtlasData[] =
{
    {12, 55, CATEGORY_EXPLORATION, FLAG_ACH_WHOLE_NEW_WORLD, sWholeNewWorldAchLabel, sWholeNewWorldAchDesc},
    {28, 55, CATEGORY_BATTLE, FLAG_ACH_ENHANCED_BATTLE, sEnhancedBattleAchLabel, sEnhancedBattleAchDesc},
    {57, 55, CATEGORY_COMPANIONS, FLAG_ACH_NEW_REGION_WHO_DIS, sNewRegionWhoDisAchLabel, sNewRegionWhoDisAchDesc},
    {43, 55, CATEGORY_QUESTING, FLAG_ACH_ADVENTURE_TIME, sAdventureTimeAchLabel, sAdventureTimeAchLabel},
    {57, 52, CATEGORY_COMPANIONS, FLAG_ACH_TRUE_LOVE, sTrueLoveAchLabel, sTrueLoveAchDesc},
    {59, 48, CATEGORY_COMPANIONS, FLAG_ACH_SILENT_STRONG_TYPE, sSilentStrongTypeAchLabel, sSilentStrongTypeAchDesc},
    {59, 50, CATEGORY_COMPANIONS, FLAG_ACH_CHILDHOOD_FRIEND, sChildhoodFriendAchLabel, sChildhoodFriendAchDesc},
    {59, 52, CATEGORY_COMPANIONS, FLAG_ACH_FIERY_PASSION, sFieryPassionAchLabel, sFieryPassionAchDesc},
    {61, 48, CATEGORY_COMPANIONS, FLAG_ACH_NERD_LOVE, sNerdLoveAchLabel, sNerdLoveAchDesc},
    {61, 50, CATEGORY_COMPANIONS, FLAG_ACH_FWB, sFWBAchLabel, sFWBAchDesc},
    {61, 52, CATEGORY_COMPANIONS, FLAG_ACH_WET_N_WILD, sWetnWildAchLabel, sWetnWildAchDesc},
    {60, 46, CATEGORY_COMPANIONS, FLAG_ACH_TRUE_ENDING, sTrueEndingAchLabel, sTrueEndingAchDesc}, 
};

#define WIN_ACH_LABEL 0
#define WIN_ACH_DESC 1

static const struct WindowTemplate sAtlasWindowTemplate[] =
{
    [WIN_ACH_LABEL] = 
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 15,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    [WIN_ACH_DESC] = 
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 14,
        .width = 28,
        .height = 5,
        .paletteNum = 15,
        .baseBlock = 31,
    },
    DUMMY_WIN_TEMPLATE
};

static const struct SpriteSheet sCursorTestTile = {
    .data = sAtlasCursorTiles,
    .size = sizeof(sAtlasCursorTiles),
    .tag = 0x6969
};
static const struct SpritePalette sCursorTestPal = {
    .data = sAtlasCursorPalette,
    .tag = 0x6969
};

const struct OamData sAtlasCursorOam =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
};

static const union AnimCmd sCursorIdleAnimation[] = {
    ANIMCMD_FRAME(0, 20),
    ANIMCMD_FRAME(4, 20),
    ANIMCMD_JUMP(0),
    ANIMCMD_END
    //ANIMCMD_FRAME(32, 4),
};

static const union AnimCmd sCursorOnAchAnimation[] = {
    ANIMCMD_FRAME(8, 1),
    ANIMCMD_END,
    //ANIMCMD_FRAME(32, 4),
};

static const union AnimCmd *const sCursorAnims[] =
{
    sCursorIdleAnimation,
    sCursorOnAchAnimation
};

const struct SpriteTemplate sAtlasCursorSpriteTemplate =
{
    .tileTag = 0x6969,
    .paletteTag = 0x6969,
    .oam = &sAtlasCursorOam,
    .anims = sCursorAnims,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = AtlasCursorSpriteCB,
};

static const struct BgTemplate sAtlasBGTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 3,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 3,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};
static void CB2_Atlas(void);
static void VBlankCB_Atlas(void);

static void Task_DrawAtlas(u8);
static void Task_HandleAtlasInput(u8);

void GiveAchievement(u32 id);
bool32 CheckAchievement(u32 id);
void TakeAchievement(u32 id);

struct AchAtlas
{
    u8 cursorSpriteId;
    s8 cursorX;
    s8 cursorY;
    s8 cursorHeldCounter;
    s8 tilemapPosX;
    s8 tilemapPosY;
    bool8 shouldUpdateTilemap;
    bool8 isOnAchTile;
};

EWRAM_DATA static struct AchAtlas sAchAtlas = {0};

void CB2_OpenAtlas(void)
{
    switch (gMain.state)
    {
    case 0:
    default:
        SetVBlankCallback(NULL);
        DmaFillLarge16(3, 0, (u8 *)VRAM, VRAM_SIZE, 0x1000);
        DmaClear32(3, OAM, OAM_SIZE);
        DmaClear16(3, PLTT, PLTT_SIZE);
        gMain.state = 1;
        break;
    case 1:
        ScanlineEffect_Stop();
        ResetTasks();
        ResetSpriteData();
        ResetPaletteFade();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 8;
        gMain.state++;
        break;
    case 2:
        CreateTask(Task_DrawAtlas, 0);
        gMain.state++;
        break;
    case 3:
        EnableInterrupts(1);
        SetVBlankCallback(VBlankCB_Atlas);
        SetMainCallback2(CB2_Atlas);
        m4aMPlayVolumeControl(&gMPlayInfo_BGM, 0xFFFF, 0x80);
        break;
    }
}

static void CB2_Atlas(void)
{
    //*(u8*)0x0203FFFF = 0x69;
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB_Atlas(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Tasks

void Task_Dummy(u8 taskId)
{

}

static bool8 IntializeAtlasPage(void);

void Task_DrawAtlas(u8 taskId)
{
    if(IntializeAtlasPage())
        gTasks[taskId].func = Task_HandleAtlasInput;
}

static bool8 IntializeAtlasPage(void)
{
    u16 * map;
    u32 i, j;
    u32 tileX, tileY;
    switch (gMain.state)
    {
    case 0:
    default:
        if (gPaletteFade.active)
            return 0;
        SetVBlankCallback(NULL);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sAtlasBGTemplates, ARRAY_COUNT(sAtlasBGTemplates));
        SetBgTilemapBuffer(3, AllocZeroed(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(2, AllocZeroed(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(1, AllocZeroed(BG_SCREEN_SIZE));
        SetBgTilemapBuffer(0, AllocZeroed(BG_SCREEN_SIZE));

        //DecompressAndLoadBgGfxUsingHeap(3, gPokedexMenu_Gfx, 0x2000, 0, 0);
        RequestDma3Copy(sAchievementAtlasTileset, (u8*)VRAM, sizeof(sAchievementAtlasTileset), 0);
        map = GetBgTilemapBuffer(3);
        sAchAtlas.tilemapPosX = 0;
        sAchAtlas.tilemapPosY = 0;
        tileY = 0;
        for(i = 0; i < 22; i++, tileY++)
        {
            tileX = 0;
            for(j = 0; j < 32; j++, tileX++)
            {
                map[i*32+j] = sAchievementAtlasTilemap[tileY*64+tileX];
            }
        }
        LoadPalette(sAchievementAtlasPalette, 0, 0x20);
        InitWindows(sAtlasWindowTemplate);
        DeactivateAllTextPrinters();
        PutWindowTilemap(0);
        CopyWindowToVram(0, 3);
        InitTextBoxGfxAndPrinters();
        gMain.state = 1;
        break;
    case 1:
        ResetSpriteData();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 8;
        LoadSpritePalette(&sCursorTestPal);
        LoadSpriteSheet(&sCursorTestTile);
        sAchAtlas.cursorSpriteId = CreateSprite(&sAtlasCursorSpriteTemplate, DISPLAY_WIDTH/2+4, DISPLAY_HEIGHT/2+4, 0);
        sAchAtlas.cursorX = 15;
        sAchAtlas.cursorY = 10;
        //LoadCompressedSpriteSheet(&sInterfaceSpriteSheet[0]);
        //LoadSpritePalettes(sInterfaceSpritePalette);
        //CreateInterfaceSprites(page);
        //AddWindow();
        //AddTextPrinterParameterized(0, 0, gText_BagIsFull, 0, 0, 0, NULL);
        gMain.state++;
        break;
    case 2:
        gMain.state++;
        break;
    case 3:
        CopyBgTilemapBufferToVram(0);
        CopyBgTilemapBufferToVram(1);
        CopyBgTilemapBufferToVram(2);
        CopyBgTilemapBufferToVram(3);
        gMain.state++;
        break;
    case 4:
        BeginNormalPaletteFade(0xFFFFFFFF, 0, 0x10, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_Atlas);
        gMain.state++;
        break;
    case 5:
        SetGpuReg(REG_OFFSET_BG0HOFS, 0);
        SetGpuReg(REG_OFFSET_BG0VOFS, 0);
        SetGpuReg(REG_OFFSET_BG1HOFS, 0);
        SetGpuReg(REG_OFFSET_BG1VOFS, 0);
        SetGpuReg(REG_OFFSET_BG2HOFS, 0);
        SetGpuReg(REG_OFFSET_BG2VOFS, 0);
        SetGpuReg(REG_OFFSET_BG3HOFS, 0);
        SetGpuReg(REG_OFFSET_BG3VOFS, 0);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        gMain.state++;
        break;
    case 6:
        if (!gPaletteFade.active)
        {
            gMain.state = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void FreeBgAndWindowBuffers(void)
{
    void* tilemapBuffer;

    FreeAllWindowBuffers();
    tilemapBuffer = GetBgTilemapBuffer(0);
    if (tilemapBuffer)
        Free(tilemapBuffer);
    tilemapBuffer = GetBgTilemapBuffer(1);
    if (tilemapBuffer)
        Free(tilemapBuffer);
    tilemapBuffer = GetBgTilemapBuffer(2);
    if (tilemapBuffer)
        Free(tilemapBuffer);
    tilemapBuffer = GetBgTilemapBuffer(3);
    if (tilemapBuffer)
        Free(tilemapBuffer);
}

#define WAIT_FAST_SCROLL 60
#define WAIT_SLOW_SCROLL 15

#define TEMP_TEST_ATLAS_WIDTH 64
#define TEMP_TEST_ATLAS_HEIGHT 64

void UpdateAtlasScroll(void);
void Task_UpdateAtlasStatus(u8 taskId);

void Task_HandleAtlasInput(u8 taskId)
{
    u32 action = 0, speed = 8;
    u8 spriteId = sAchAtlas.cursorSpriteId;
    
    //StartSpriteAnim(&gSprites[spriteId], 1);
    
    DecideActionFromInput(&action);

    sAchAtlas.shouldUpdateTilemap = FALSE;
    if(!gTasks[taskId].data[1])
    {
        switch(action)
        {
        case 0:
        default:
            break;
        case 1:
            gSprites[spriteId].data[0] = action;
            gSprites[spriteId].data[1] = speed;
            sAchAtlas.cursorX -= speed / 8;
            if(sAchAtlas.cursorX < 1)
                sAchAtlas.cursorX = 1;
            if(sAchAtlas.tilemapPosX != 0 && sAchAtlas.cursorX <= (10 + sAchAtlas.tilemapPosX))
            {
                gSprites[spriteId].data[0] = 0;
                //sAchAtlas.cursorX = 10;
                sAchAtlas.tilemapPosX -= speed / 8;
                if(sAchAtlas.tilemapPosX < 0)
                    sAchAtlas.tilemapPosX = 0;
                sAchAtlas.shouldUpdateTilemap = TRUE;
            }
            break;
        case 2:
            gSprites[spriteId].data[0] = action;
            gSprites[spriteId].data[1] = speed;
            sAchAtlas.cursorX += speed / 8;
            if(sAchAtlas.cursorX >= TEMP_TEST_ATLAS_WIDTH-2)
                sAchAtlas.cursorX = TEMP_TEST_ATLAS_WIDTH-2;
            if(sAchAtlas.tilemapPosX != TEMP_TEST_ATLAS_WIDTH-30 && sAchAtlas.cursorX >= (20 + sAchAtlas.tilemapPosX))
            {
                gSprites[spriteId].data[0] = 0;
                //sAchAtlas.cursorX = 20;
                sAchAtlas.tilemapPosX += speed / 8;
                if(sAchAtlas.tilemapPosX >= TEMP_TEST_ATLAS_WIDTH-30)
                    sAchAtlas.tilemapPosX = TEMP_TEST_ATLAS_WIDTH-30;
                sAchAtlas.shouldUpdateTilemap = TRUE;
            }
            break;
        case 3:
            gSprites[spriteId].data[0] = action;
            gSprites[spriteId].data[1] = speed;
            sAchAtlas.cursorY -= speed / 8;
            if(sAchAtlas.cursorY < 1)
                sAchAtlas.cursorY = 1;
            if(sAchAtlas.tilemapPosY != 0 && sAchAtlas.cursorY <= (5 + sAchAtlas.tilemapPosY))
            {
                gSprites[spriteId].data[0] = 0;
                //sAchAtlas.cursorY = 5;
                sAchAtlas.tilemapPosY -= speed / 8;
                if(sAchAtlas.tilemapPosY < 0)
                    sAchAtlas.tilemapPosY = 0;
                sAchAtlas.shouldUpdateTilemap = TRUE;
            }
            break;
        case 4:
            gSprites[spriteId].data[0] = action;
            gSprites[spriteId].data[1] = speed;
            sAchAtlas.cursorY += speed / 8;
            if(sAchAtlas.cursorY >= TEMP_TEST_ATLAS_HEIGHT-2)
                sAchAtlas.cursorY = TEMP_TEST_ATLAS_HEIGHT-2;
            if(sAchAtlas.tilemapPosY != TEMP_TEST_ATLAS_HEIGHT-20 && sAchAtlas.cursorY >= (15 + sAchAtlas.tilemapPosY))
            {
                gSprites[spriteId].data[0] = 0;
                //sAchAtlas.cursorY = 15;
                sAchAtlas.tilemapPosY += speed / 8;
                if(sAchAtlas.tilemapPosY >= TEMP_TEST_ATLAS_HEIGHT-20)
                    sAchAtlas.tilemapPosY = TEMP_TEST_ATLAS_HEIGHT-20;
                sAchAtlas.shouldUpdateTilemap = TRUE;   
            }
            break;
        }
    }
    UpdateAtlasScroll();
    if(action != 0)
    {
        gTasks[taskId].data[0] = action;
        gTasks[taskId].func = Task_UpdateAtlasStatus;
    }
}

void Task_UpdateAtlasStatus(u8 taskId)
{
    u32 action = gTasks[taskId].data[0];
    u8 spriteId = sAchAtlas.cursorSpriteId;
    u32 achTileId;
    u32 i;
    u32 stringWidth;
    if(sAchAtlas.isOnAchTile)
    {
        if(action == 6 && gTasks[taskId].data[1])
        {
            ClearStdWindowAndFrameToTransparent(WIN_ACH_DESC, TRUE);
            gTasks[taskId].data[1] = FALSE;
            gTasks[taskId].func = Task_HandleAtlasInput;
            return;
        }
        else if(action == 5 && !gTasks[taskId].data[1])
        {
            for(i = 0; i < ARRAY_COUNT(sTestAtlasData); i++)
            {
                if(sTestAtlasData[i].x == sAchAtlas.cursorX && sTestAtlasData[i].y == sAchAtlas.cursorY)
                {
                    
                    gTasks[taskId].data[1] = TRUE; 
                    DrawStdWindowFrame(WIN_ACH_DESC, TRUE);
                    if(CheckAchievement(sTestAtlasData[i].flagId))
                    {
                        StringExpandPlaceholders(gStringVar4, sTestAtlasData[i].descString);
                        AddTextPrinterParameterized3(WIN_ACH_DESC, 0, 0, 0, sTextColors[sTestAtlasData[i].category], 0, gStringVar4);
                    }
                    else
                    {
                        AddTextPrinterParameterized3(WIN_ACH_DESC, 0, 0, 0, sTextColors[0], 0, sDescriptionNotAvailable);
                    }
                    
                }
            }
            gTasks[taskId].func = Task_HandleAtlasInput;
            return;
        }
        else if((action == 6 && !gTasks[taskId].data[1]) || (action == 5 && gTasks[taskId].data[1]))
        {
            gTasks[taskId].func = Task_HandleAtlasInput;
            return;
        }
        else if(gTasks[taskId].data[1])
        {
            gTasks[taskId].func = Task_HandleAtlasInput;
            return;
        }
    }
    for(i = 0; i < ARRAY_COUNT(sTestAtlasData); i++)
    {
        if(sTestAtlasData[i].x == sAchAtlas.cursorX && sTestAtlasData[i].y == sAchAtlas.cursorY)
        {
            StartSpriteAnim(&gSprites[spriteId], 1);
            ClearStdWindowAndFrameToTransparent(WIN_ACH_LABEL, TRUE);
            ClearStdWindowAndFrameToTransparent(WIN_ACH_DESC, TRUE);
            if(sAchAtlas.cursorY >= (13 + sAchAtlas.tilemapPosY))
            {
                SetWindowAttribute(WIN_ACH_LABEL, WINDOW_TILEMAP_TOP, 17);
                SetWindowAttribute(WIN_ACH_DESC, WINDOW_TILEMAP_TOP, 1);
            }
            else
            {
                SetWindowAttribute(WIN_ACH_LABEL, WINDOW_TILEMAP_TOP, 1);
                SetWindowAttribute(WIN_ACH_DESC, WINDOW_TILEMAP_TOP, 14);
            }
            stringWidth = GetStringWidth(0, sTestAtlasData[i].nameString, 0);
            SetWindowAttribute(WIN_ACH_LABEL, WINDOW_WIDTH, (stringWidth + 7) / 8);
            DrawStdWindowFrame(WIN_ACH_LABEL, TRUE);
            sAchAtlas.isOnAchTile = TRUE;
            AddTextPrinterParameterized3(WIN_ACH_LABEL, 0, 0, 0, sTextColors[sTestAtlasData[i].category], 0, sTestAtlasData[i].nameString);
            achTileId = i;
            break;
        }
        achTileId = -1u;
    }
    if(achTileId == -1u)
    {
        if(sAchAtlas.isOnAchTile)
        {
            ClearStdWindowAndFrameToTransparent(WIN_ACH_LABEL, TRUE);
            ClearStdWindowAndFrameToTransparent(WIN_ACH_DESC, TRUE);
        }
        sAchAtlas.isOnAchTile = FALSE;
        StartSpriteAnimIfDifferent(&gSprites[spriteId], 0);
    }

    gTasks[taskId].func = Task_HandleAtlasInput;
}

void DecideActionFromInput(u32 * action)
{
    if(JOY_NEW(SELECT_BUTTON) && JOY_HELD(B_BUTTON) && JOY_HELD(A_BUTTON))
    {
        GiveAchievement(0);
        GiveAchievement(1);
        GiveAchievement(2);
        GiveAchievement(3);
        GiveAchievement(4);
    }
    if(JOY_NEW(DPAD_LEFT))
        *action = 1;
    else if(JOY_HELD(DPAD_LEFT)) 
        if(sAchAtlas.cursorHeldCounter > WAIT_FAST_SCROLL)
        {
            *action = 1;
            sAchAtlas.cursorHeldCounter = WAIT_FAST_SCROLL-1;
        }
        else if((sAchAtlas.cursorHeldCounter % WAIT_SLOW_SCROLL) == 0)
        {
            *action = 1;
            sAchAtlas.cursorHeldCounter++;
        }
        else
            sAchAtlas.cursorHeldCounter++;
    else if(JOY_NEW(DPAD_RIGHT))
        *action = 2;
    else if(JOY_HELD(DPAD_RIGHT)) 
        if(sAchAtlas.cursorHeldCounter > WAIT_FAST_SCROLL)
        {
            *action = 2;
            sAchAtlas.cursorHeldCounter = WAIT_FAST_SCROLL-1;
        }
        else if((sAchAtlas.cursorHeldCounter % WAIT_SLOW_SCROLL) == 0)
        {
            *action = 2;
            sAchAtlas.cursorHeldCounter++;
        }
        else
            sAchAtlas.cursorHeldCounter++;
    else if(JOY_NEW(DPAD_UP))
        *action = 3;
    else if(JOY_HELD(DPAD_UP)) 
        if(sAchAtlas.cursorHeldCounter > WAIT_FAST_SCROLL)
        {
            *action = 3;
            sAchAtlas.cursorHeldCounter = WAIT_FAST_SCROLL-1;
        }
        else if((sAchAtlas.cursorHeldCounter % WAIT_SLOW_SCROLL) == 0)
        {
            *action = 3;
            sAchAtlas.cursorHeldCounter++;
        }
        else
            sAchAtlas.cursorHeldCounter++;
    else if(JOY_NEW(DPAD_DOWN))
        *action = 4;
    else if(JOY_HELD(DPAD_DOWN)) 
        if(sAchAtlas.cursorHeldCounter > WAIT_FAST_SCROLL)
        {
            *action = 4;
            sAchAtlas.cursorHeldCounter = WAIT_FAST_SCROLL-1;
        }
        else if((sAchAtlas.cursorHeldCounter % WAIT_SLOW_SCROLL) == 0)
        {
            *action = 4;
            sAchAtlas.cursorHeldCounter++;
        }
        else
            sAchAtlas.cursorHeldCounter++;
    else
    {
        sAchAtlas.cursorHeldCounter -= 5;
        if(sAchAtlas.cursorHeldCounter < 10) sAchAtlas.cursorHeldCounter = 1;
    }

    if(JOY_NEW(A_BUTTON))
        *action = 5;
    else if(JOY_NEW(B_BUTTON))
        *action = 6;
}

void UpdateAtlasScroll(void)
{
    u16 * map;
    u32 i, j;
    u32 tileX, tileY;

    if(sAchAtlas.shouldUpdateTilemap)
    {
        map = GetBgTilemapBuffer(3);
        tileY = sAchAtlas.tilemapPosY;
        for(i = 0; i < 22; i++, tileY++)
        {
            tileX = sAchAtlas.tilemapPosX;
            for(j = 0; j < 32; j++, tileX++)
            {
                map[i*32+j] = sAchievementAtlasTilemap[tileY*64+tileX];
            }
        }
        CopyBgTilemapBufferToVram(3);
    }
}


void AtlasCursorSpriteCB(struct Sprite *sprite)
{
    u32 action = sprite->data[0];
    u32 speed = sprite->data[1];
    switch(action)
    {
        case 0:
        default:
            break;
        case 1:
            sprite->pos1.x -= speed;
            if(sprite->pos1.x < 8)
                sprite->pos1.x = 12;
            break;
        case 2:
            sprite->pos1.x += speed;
            if(sprite->pos1.x > DISPLAY_WIDTH-12)
                sprite->pos1.x = DISPLAY_WIDTH-12;
            break;
        case 3:
            sprite->pos1.y -= speed;
            if(sprite->pos1.y < 8)
                sprite->pos1.y = 12;
            break;
        case 4:
            sprite->pos1.y += speed;
            if(sprite->pos1.y > DISPLAY_HEIGHT-12)
                sprite->pos1.y = DISPLAY_HEIGHT-12;
            break;
    }
    sprite->data[0] = 0;
}

void GiveAchievement(u32 id)
{
    if(id > ACH_FLAGS_COUNT)
        return;

    gSaveBlock2Ptr->achFlags[id / 8] |= 1 << (id % 8);
}

bool32 CheckAchievement(u32 id)
{
    if(id > ACH_FLAGS_COUNT)
        return FALSE;

    return !!((gSaveBlock2Ptr->achFlags[id / 8] >> (id % 8)) & 1);
}

void TakeAchievement(u32 id)
{
    if(id > ACH_FLAGS_COUNT)
        return;
        
    gSaveBlock2Ptr->achFlags[id / 8] &= ~(1 << (id % 8));
}