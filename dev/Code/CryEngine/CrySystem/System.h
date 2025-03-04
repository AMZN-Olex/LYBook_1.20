/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include <ISystem.h>
#include <IRenderer.h>
#include <IPhysics.h>
#include <IPlatformOS.h>
#include <IWindowMessageHandler.h>

#include "Timer.h"
#include <CryVersion.h>
#include "CmdLine.h"
#include "CryName.h"

#include "FrameProfileSystem.h"
#include "MTSafeAllocator.h"
#include "CPUDetect.h"
#include "PakVars.h"
#include "MemoryFragmentationProfiler.h"    // CMemoryFragmentationProfiler
#include "ThreadTask.h"
#include "RenderBus.h"

#include <LoadScreenBus.h>
#include <ThermalInfo.h>

#include <AzCore/Module/DynamicModuleHandle.h>

struct IConsoleCmdArgs;
class CServerThrottle;
struct ICryFactoryRegistryImpl;
struct IZLibCompressor;
class CLoadingProfilerSystem;
class CWatchdogThread;
class CThreadManager;

struct ICryPerfHUD;
namespace minigui
{
    struct IMiniGUI;
}

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define SYSTEM_H_SECTION_1 1
#define SYSTEM_H_SECTION_2 2
#define SYSTEM_H_SECTION_3 3
#define SYSTEM_H_SECTION_4 4
#endif

#if defined(ANDROID)
#define USE_ANDROIDCONSOLE
#elif defined(MAC) // || defined(LINUX)
#define USE_UNIXCONSOLE
#elif defined(IOS) || defined(APPLETV)
#define USE_IOSCONSOLE
#elif defined(WIN32) || defined(WIN64)
#define USE_WINDOWSCONSOLE
#else
#define USE_NULLCONSOLE
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_1
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_h_provo.inl"
    #endif
#else
#if defined(WIN32) || defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_ALLOW_CREATE_BACKUP_LOG_FILE 1
#endif
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEFINE_DETECT_PROCESSOR 1
#endif

//////////////////////////////////////////////////////////////////////////
#if !defined(LINUX) && !defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_CREATE_AVI_READER 1
#endif
#if defined(WIN32) || defined(APPLE) || defined(LINUX)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DO_PREASSERT 1
#endif
#if defined(MAC) || (defined(LINUX) && !defined(ANDROID))
#define AZ_LEGACY_CRYSYSTEM_TRAIT_ASM_VOLATILE_CPUID 1
#endif
#if (defined(WIN32) && !defined(WIN64)) || (defined(LINUX) && !defined(ANDROID) && !defined(LINUX64))
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HAS64BITEXT 1
#endif
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HTSUPPORTED 1
#endif
#if defined(WIN32) || (defined(LINUX) && !defined(ANDROID)) || defined(MAC)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HASCPUID 1
#endif
#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_HASAFFINITYMASK 1
#endif

#if defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_CRYPAK_POSIX 1
#endif

#if defined(WIN64)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_BIT64 1
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_PACKED_PEHEADER 1
#endif
#if defined(WIN32) || defined(LINUX)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_RENDERMEMORY_INFO 1
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_HANDLER_SYNC_AFFINITY 1
#endif

#if defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_FORWARD_EXCEPTION_POINTERS 1
#endif

#if !defined(_WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_SINGLETON 1
#endif
#if !defined(LINUX) && !defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_TRANSLATE 1
#endif
#if !defined(LINUX) && !defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_DEBUGCALLSTACK_APPEND_MODULENAME 1
#endif

#if !(defined(ANDROID) || defined(IOS) || defined(APPLETV) || defined(LINUX))
#define AZ_LEGACY_CRYSYSTEM_TRAIT_IMAGEHANDLER_TIFFIO 1
#endif

#if 1
#define AZ_LEGACY_CRYSYSTEM_TRAIT_JOBMANAGER_SIXWORKERTHREADS 0
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_MEMADDRESSRANGE_WINDOWS_STYLE 1
#endif

#if 1
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_EXCLUDEUPDATE_ON_CONSOLE 0
#endif
#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_MESSAGE_HANDLER 1
#endif
#if defined(WIN64) || defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_CAPTURESTACK 1
#endif

#if !defined(LINUX) && !defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_SYSTEMCFG_MODULENAME 1
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_RENDERTHREADINFO 1
#else
#define AZ_LEGACY_CRYSYSTEM_TRAIT_RENDERTHREADINFO 0
#endif

#if defined(WIN32) || defined(WIN64)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_THREADINFO_WINDOWS_STYLE 1
#endif

#if defined(WIN32)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_THREADTASK_EXCEPTIONS 1
#endif
//////////////////////////////////////////////////////////////////////////

#if !defined(LINUX)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_SIMULATE_TASK 1
#endif

#if defined(APPLE) || defined(LINUX)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_FACTORY_REGISTRY_USE_PRINTF_FOR_FATAL 1
#endif

#if defined(LINUX) || defined(APPLE)
#define AZ_LEGACY_CRYSYSTEM_TRAIT_USE_FTELL_NOT_FTELLI64 1
#endif

#endif

#if defined(USE_UNIXCONSOLE) || defined(USE_ANDROIDCONSOLE) || defined(USE_WINDOWSCONSOLE) || defined(USE_IOSCONSOLE) || defined(USE_NULLCONSOLE)
#define USE_DEDICATED_SERVER_CONSOLE
#endif

#if AZ_LEGACY_CRYSYSTEM_TRAIT_CREATE_AVI_READER
#define DOWNLOAD_MANAGER
#endif

#ifdef DOWNLOAD_MANAGER
#include "DownloadManager.h"
#endif //DOWNLOAD_MANAGER


#if defined(LINUX)
    #include "CryLibrary.h"
#endif

#define NUM_UPDATE_TIMES (128U)

#ifdef WIN32
typedef void* WIN_HMODULE;
#else
typedef void* WIN_HMODULE;
#endif

#if !defined(CRY_ASYNC_MEMCPY_DELEGATE_TO_CRYSYSTEM)
CRY_ASYNC_MEMCPY_API void cryAsyncMemcpy(void* dst, const void* src, size_t size, int nFlags, volatile int* sync);
#else
CRY_ASYNC_MEMCPY_API void cryAsyncMemcpyDelegate(void* dst, const void* src, size_t size, int nFlags, volatile int* sync);
#endif


//forward declarations
class CScriptSink;
namespace Audio
{
    struct IAudioSystem;
    struct IMusicSystem;
} // namespace Audio
struct SDefaultValidator;
struct IDataProbe;
class CPhysRenderer;
class CVisRegTest;
class CThreadProfiler;

#define PHSYICS_OBJECT_ENTITY 0

typedef void (__cdecl * VTuneFunction)(void);
extern VTuneFunction VTResume;
extern VTuneFunction VTPause;

#define MAX_STREAMING_POOL_INDEX 6
#define MAX_THREAD_POOL_INDEX 6

struct SSystemCVars
{
    int sys_streaming_requests_grouping_time_period;
    int sys_streaming_sleep;
    int sys_streaming_memory_budget;
    int sys_streaming_max_finalize_per_frame;
    float sys_streaming_max_bandwidth;
    int sys_streaming_cpu;
    int sys_streaming_cpu_worker;
    int sys_streaming_debug;
    int sys_streaming_resetstats;
    int sys_streaming_debug_filter;
    float sys_streaming_debug_filter_min_time;
    int sys_streaming_use_optical_drive_thread;
    ICVar* sys_streaming_debug_filter_file_name;
    ICVar* sys_localization_folder;
    int sys_streaming_in_blocks;

    int sys_float_exceptions;
    int sys_no_crash_dialog;
    int sys_dump_aux_threads;
    int sys_WER;
    int sys_dump_type;
    int sys_ai;
    int sys_physics;
    int sys_entitysystem;
    int sys_trackview;
    int sys_vtune;
    float sys_update_profile_time;
    int sys_limit_phys_thread_count;
    int sys_usePlatformSavingAPI;
#ifndef _RELEASE
    int sys_usePlatformSavingAPIEncryption;
#endif
    int sys_MaxFPS;
    float sys_maxTimeStepForMovieSystem;
    int sys_force_installtohdd_mode;

#ifdef USE_HTTP_WEBSOCKETS
    int sys_simple_http_base_port;
#endif

    int sys_asserts;
    int sys_error_debugbreak;

    int sys_rendersplashscreen;
    const char* sys_splashscreen;

    enum SplashScreenScaleMode
    {
        SplashScreenScaleMode_Fit = 0,
        SplashScreenScaleMode_Fill
    };
    int sys_splashScreenScaleMode;

    int sys_deferAudioUpdateOptim;
#if USE_STEAM
#ifndef RELEASE
    int sys_steamAppId;
#endif // RELEASE
    int sys_useSteamCloudForPlatformSaving;
#endif // USE_STEAM

    PakVars pakVars;

#if defined(WIN32)
    int sys_display_threads;
#elif defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_2
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_h_provo.inl"
    #endif
#endif
};
extern SSystemCVars g_cvars;

class CSystem;

struct SmallModuleInfo
{
    string name;
    CryModuleMemoryInfo memInfo;
};

struct SCryEngineStatsModuleInfo
{
    string name;
    CryModuleMemoryInfo memInfo;
    uint32 moduleStaticSize;
    uint32 usedInModule;
    uint32 SizeOfCode;
    uint32 SizeOfInitializedData;
    uint32 SizeOfUninitializedData;
};

struct SCryEngineStatsGlobalMemInfo
{
    int totalUsedInModules;
    int totalCodeAndStatic;
    int countedMemoryModules;
    uint64 totalAllocatedInModules;
    int totalNumAllocsInModules;
    std::vector<SCryEngineStatsModuleInfo> modules;
};

struct CProfilingSystem
    : public IProfilingSystem
{
    //////////////////////////////////////////////////////////////////////////
    // VTune Profiling interface.

    // Summary:
    //   Resumes vtune data collection.
    virtual void VTuneResume();
    // Summary:
    //   Pauses vtune data collection.
    virtual void VTunePause();
    //////////////////////////////////////////////////////////////////////////
};

class AssetSystem;

/*
===========================================
The System interface Class
===========================================
*/
class CXConsole;
namespace Telemetry {
    class CFileStream;
    class CUDPStream;
}
//////////////////////////////////////////////////////////////////////
//! ISystem implementation
class CSystem
    : public ISystem
    , public ILoadConfigurationEntrySink
    , public ISystemEventListener
    , public IWindowMessageHandler
    , public AZ::RenderNotificationsBus::Handler
    , public CrySystemRequestBus::Handler
{
public:

    inline void* operator new(std::size_t)
    {
        size_t allocated = 0;
        return CryMalloc(sizeof(CSystem), allocated, 64);
    }
    inline void operator delete(void* p)
    {
        CryFree(p, 64);
    }

    CSystem(SharedEnvironmentInstance* pSharedEnvironment);
    ~CSystem();

    static void OnLanguageCVarChanged(ICVar* language);
    static void OnLanguageAudioCVarChanged(ICVar* language);
    static void OnLocalizationFolderCVarChanged(ICVar* const pLocalizationFolder);

    // interface ILoadConfigurationEntrySink ----------------------------------

    virtual void OnLoadConfigurationEntry(const char* szKey, const char* szValue, const char* szGroup);

    // ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);

    ///////////////////////////////////////////////////////////////////////////
    //! @name ISystem implementation
    //@{
    virtual bool Init(const SSystemInitParams& startupParams);
    virtual void Release();

    virtual SSystemGlobalEnvironment* GetGlobalEnvironment() { return &m_env; }

    virtual bool UpdatePreTickBus(int updateFlags = 0, int nPauseMode = 0);
    virtual bool UpdatePostTickBus(int updateFlags = 0, int nPauseMode = 0);
    virtual bool UpdateLoadtime();
    virtual void DoWorkDuringOcclusionChecks();
    virtual bool NeedDoWorkDuringOcclusionChecks() { return m_bNeedDoWorkDuringOcclusionChecks; }

    //! Begin rendering frame.
    void    RenderBegin();
    //! Render subsystems.
    void    Render();
    //! End rendering frame and swap back buffer.
    void    RenderEnd(bool bRenderStats = true, bool bMainWindow = true);

    //Called when the renderer finishes rendering the scene
    void OnScene3DEnd() override;

    ////////////////////////////////////////////////////////////////////////
    // CrySystemRequestBus interface implementation
    ISystem* GetCrySystem() override;
    ////////////////////////////////////////////////////////////////////////

    //! Update screen during loading.
    void UpdateLoadingScreen();

    //! Update screen and call some important tick functions during loading.
    void SynchronousLoadingTick(const char* pFunc, int line);

    //! Renders the statistics; this is called from RenderEnd, but if the
    //! Host application (Editor) doesn't employ the Render cycle in ISystem,
    //! it may call this method to render the essential statistics
    void RenderStatistics () override;
    void RenderPhysicsHelpers() override;
    void RenderPhysicsStatistics(IPhysicalWorld* pWorld) override;

    uint32 GetUsedMemory();

    //! For asset processor, we need to know what kind of assets we're loading.  This comes all the way from bootstrap.cfg
    //! It will be a string like "pc" or "es3" or such and controls what kind of assets we have access to (its used when for example
    //! attempting to open a file when multiple different assets for different platforms are available.)
    const char* GetAssetsPlatform() const;

    virtual void DumpMemoryUsageStatistics(bool bUseKB);
    virtual void DumpMemoryCoverage();
    void CollectMemInfo(SCryEngineStatsGlobalMemInfo&);

#ifndef _RELEASE
    virtual void GetCheckpointData(ICheckpointData& data);
    virtual void IncreaseCheckpointLoadCount();
    virtual void SetLoadOrigin(LevelLoadOrigin origin);
#endif

    virtual bool SteamInit();

    void Relaunch(bool bRelaunch);
    bool IsRelaunch() const { return m_bRelaunch; };
    bool LaunchAssetProcessor() override;
    void SerializingFile(int mode) { m_iLoadingMode = mode; }
    int  IsSerializingFile() const { return m_iLoadingMode; }
    void Quit();
    bool IsQuitting() const;
    void ShutdownFileSystem(); // used to cleanup any file resources, such as cache handle.
    bool IsShaderCacheGenMode() const { return m_bShaderCacheGenMode; }
    void SetAffinity();
    virtual const char* GetUserName();
    virtual int GetApplicationInstance();
    virtual sUpdateTimes& GetCurrentUpdateTimeStats();
    virtual const sUpdateTimes* GetUpdateTimeStats(uint32&, uint32&);

    IGame* GetIGame(){ return m_env.pGame; }
    IRenderer* GetIRenderer(){ return m_env.pRenderer; }
    IInput* GetIInput(){ return m_env.pInput; }
    ITimer* GetITimer(){ return m_env.pTimer; }
    INetwork* GetINetwork(){ return m_env.pNetwork; }
    ICryPak* GetIPak() { return m_env.pCryPak; };
    IConsole* GetIConsole() { return m_env.pConsole; };
    IRemoteConsole* GetIRemoteConsole();
    IScriptSystem* GetIScriptSystem(){ return m_env.pScriptSystem; }
    I3DEngine* GetI3DEngine(){ return m_env.p3DEngine; }
    ICharacterManager* GetIAnimationSystem() {return m_env.pCharacterManager; }
    IPhysicalWorld* GetIPhysicalWorld(){ return m_env.pPhysicalWorld; }
    IMovieSystem* GetIMovieSystem() { return m_env.pMovieSystem; };
    IAISystem* GetAISystem(){ return m_env.pAISystem; }
    IMemoryManager* GetIMemoryManager(){ return m_pMemoryManager; }
    IEntitySystem* GetIEntitySystem(){ return m_env.pEntitySystem; }
    IThreadManager* GetIThreadManager() override {return m_env.pThreadManager; }
    ICryFont* GetICryFont(){ return m_env.pCryFont; }
    ILog* GetILog(){ return m_env.pLog; }
    ICmdLine* GetICmdLine(){ return m_pCmdLine; }
    IStreamEngine* GetStreamEngine();
    IValidator* GetIValidator() { return m_pValidator; };
    IPhysicsDebugRenderer* GetIPhysicsDebugRenderer();
    IPhysRenderer* GetIPhysRenderer();
    IFrameProfileSystem* GetIProfileSystem() { return &m_FrameProfileSystem; }
    virtual IDiskProfiler* GetIDiskProfiler() { return m_pDiskProfiler; }
    CThreadProfiler* GetThreadProfiler() { return m_pThreadProfiler; }
    INameTable* GetINameTable() { return m_env.pNameTable; };
    IViewSystem* GetIViewSystem();
    ILevelSystem* GetILevelSystem();
    IBudgetingSystem* GetIBudgetingSystem()  { return(m_pIBudgetingSystem); }
    IFlowSystem* GetIFlowSystem() { return m_env.pFlowSystem; }
    IDialogSystem* GetIDialogSystem() { return m_env.pDialogSystem; }
    DRS::IDynamicResponseSystem* GetIDynamicResponseSystem() { return m_env.pDynamicResponseSystem; }
    IHardwareMouse* GetIHardwareMouse() { return m_env.pHardwareMouse; }
    ISystemEventDispatcher* GetISystemEventDispatcher() { return m_pSystemEventDispatcher; }
    ITestSystem* GetITestSystem() { return m_pTestSystem; }
    IThreadTaskManager* GetIThreadTaskManager();
    IResourceManager* GetIResourceManager();
    ITextModeConsole* GetITextModeConsole();
    IFileChangeMonitor* GetIFileChangeMonitor() { return m_env.pFileChangeMonitor; }
    IVisualLog* GetIVisualLog() { return m_env.pVisualLog; }
    INotificationNetwork* GetINotificationNetwork() { return m_pNotificationNetwork; }
    IProfilingSystem* GetIProfilingSystem() { return &m_ProfilingSystem; }
    IPlatformOS* GetPlatformOS() { return m_pPlatformOS.get(); }
    ICryPerfHUD* GetPerfHUD() { return m_pPerfHUD; }
    IZLibCompressor* GetIZLibCompressor() { return m_pIZLibCompressor; }
    IZLibDecompressor* GetIZLibDecompressor() { return m_pIZLibDecompressor; }
    ILZ4Decompressor* GetLZ4Decompressor() { return m_pILZ4Decompressor; }
    WIN_HWND GetHWND(){ return m_hWnd; }
    IGemManager* GetGemManager() override { return m_pGemManager; }
    ICrypto* GetCrypto() { return m_crypto; }
    //////////////////////////////////////////////////////////////////////////
    // retrieves the perlin noise singleton instance
    CPNoise3* GetNoiseGen();
    virtual uint64 GetUpdateCounter() { return m_nUpdateCounter; };

    virtual void SetLoadingProgressListener(ILoadingProgressListener* pLoadingProgressListener)
    {
        m_pProgressListener = pLoadingProgressListener;
    };

    virtual ILoadingProgressListener* GetLoadingProgressListener() const
    {
        return m_pProgressListener;
    };

    void        SetIGame(IGame* pGame) {m_env.pGame = pGame; }
    void    SetIFlowSystem(IFlowSystem* pFlowSystem) { m_env.pFlowSystem = pFlowSystem; }
    void    SetIDialogSystem(IDialogSystem* pDialogSystem) { m_env.pDialogSystem = pDialogSystem; }
    void SetIDynamicResponseSystem(DRS::IDynamicResponseSystem* pDynamicResponseSystem) { m_env.pDynamicResponseSystem = pDynamicResponseSystem; }
    void    SetIMaterialEffects(IMaterialEffects* pMaterialEffects) { m_env.pMaterialEffects = pMaterialEffects; }
    void        SetIParticleManager(IParticleManager* pParticleManager) { m_env.pParticleManager = pParticleManager; }
    void        SetIOpticsManager(IOpticsManager* pOpticsManager) { m_env.pOpticsManager = pOpticsManager; }
    void    SetIFileChangeMonitor(IFileChangeMonitor* pFileChangeMonitor) { m_env.pFileChangeMonitor = pFileChangeMonitor; }
    void    SetIVisualLog(IVisualLog* pVisualLog) { m_env.pVisualLog = pVisualLog; }
    void        DetectGameFolderAccessRights();

    virtual void ExecuteCommandLine(bool deferred=true);

    virtual void GetUpdateStats(SSystemUpdateStats& stats);

    //////////////////////////////////////////////////////////////////////////
    virtual XmlNodeRef CreateXmlNode(const char* sNodeName = "", bool bReuseStrings = false, bool bIsProcessingInstruction = false);
    virtual XmlNodeRef LoadXmlFromFile(const char* sFilename, bool bReuseStrings = false);
    virtual XmlNodeRef LoadXmlFromBuffer(const char* buffer, size_t size, bool bReuseStrings = false, bool bSuppressWarnings = false);
    virtual IXmlUtils* GetXmlUtils();
    //////////////////////////////////////////////////////////////////////////

    virtual Serialization::IArchiveHost* GetArchiveHost() const { return m_pArchiveHost; }

    void SetViewCamera(CCamera& Camera){ m_ViewCamera = Camera; }
    CCamera& GetViewCamera() { return m_ViewCamera; }

    virtual int GetCPUFlags()
    {
        int Flags = 0;
        if (!m_pCpu)
        {
            return Flags;
        }
        if (m_pCpu->hasMMX())
        {
            Flags |= CPUF_MMX;
        }
        if (m_pCpu->hasSSE())
        {
            Flags |= CPUF_SSE;
        }
        if (m_pCpu->hasSSE2())
        {
            Flags |= CPUF_SSE2;
        }
        if (m_pCpu->has3DNow())
        {
            Flags |= CPUF_3DNOW;
        }
        if (m_pCpu->hasF16C())
        {
            Flags |= CPUF_F16C;
        }

        return Flags;
    }
    virtual int GetLogicalCPUCount()
    {
        if (m_pCpu)
        {
            return m_pCpu->GetLogicalCPUCount();
        }
        return 0;
    }

    void IgnoreUpdates(bool bIgnore) { m_bIgnoreUpdates = bIgnore; };

    void SetIProcess(IProcess* process);
    IProcess* GetIProcess(){ return m_pProcess; }

    bool IsTestMode() const { return m_bTestMode; }
    //@}

    void SleepIfNeeded();

    virtual void DisplayErrorMessage(const char* acMessage, float fTime, const float* pfColor = 0, bool bHardError = true);

    virtual void FatalError(const char* format, ...) PRINTF_PARAMS(2, 3);
    virtual void ReportBug(const char* format, ...) PRINTF_PARAMS(2, 3);
    // Validator Warning.
    void WarningV(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args);
    void Warning(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, ...);
    virtual int ShowMessage(const char* text, const char* caption, unsigned int uType);
    bool CheckLogVerbosity(int verbosity);

    virtual void DebugStats(bool checkpoint, bool leaks);
    void DumpWinHeaps();

    virtual int DumpMMStats(bool log);

    //! Return pointer to user defined callback.
    ISystemUserCallback* GetUserCallback() const { return m_pUserCallback; };
#if defined(CVARS_WHITELIST)
    virtual ICVarsWhitelist* GetCVarsWhiteList() const { return m_pCVarsWhitelist; };
    virtual ILoadConfigurationEntrySink* GetCVarsWhiteListConfigSink() const { return m_pCVarsWhitelistConfigSink; }
#else
    virtual ILoadConfigurationEntrySink* GetCVarsWhiteListConfigSink() const { return NULL; }
#endif // defined(CVARS_WHITELIST)

    //////////////////////////////////////////////////////////////////////////
    virtual void SaveConfiguration();
    virtual void LoadConfiguration(const char* sFilename, ILoadConfigurationEntrySink* pSink = 0, bool warnIfMissing = true);
    virtual ESystemConfigSpec GetConfigSpec(bool bClient = true);
    virtual void SetConfigSpec(ESystemConfigSpec spec, ESystemConfigPlatform platform, bool bClient);
    virtual ESystemConfigSpec GetMaxConfigSpec() const;
    virtual ESystemConfigPlatform GetConfigPlatform() const;
    virtual void SetConfigPlatform(const ESystemConfigPlatform platform);
    //////////////////////////////////////////////////////////////////////////

    virtual int SetThreadState(ESubsystem subsys, bool bActive);
    virtual ICrySizer* CreateSizer();
    virtual bool IsPaused() const { return m_bPaused; };

    //////////////////////////////////////////////////////////////////////////
    // Creates an instance of the AVI Reader class.
    virtual IAVI_Reader* CreateAVIReader();
    // Release the AVI reader
    virtual void ReleaseAVIReader(IAVI_Reader* pAVIReader);

    virtual ILocalizationManager* GetLocalizationManager();
    virtual void debug_GetCallStack(const char** pFunctions, int& nCount);
    virtual void debug_LogCallStack(int nMaxFuncs = 32, int nFlags = 0);
    // Get the current callstack in raw address form (more lightweight than the above functions)
    // static as memReplay needs it before CSystem has been setup - expose a ISystem interface to this function if you need it outside CrySystem
    static  void debug_GetCallStackRaw(void** callstack, uint32& callstackLength);
    virtual void ApplicationTest(const char* szParam);

    virtual ICryFactoryRegistry* GetCryFactoryRegistry() const;

public:
    // this enumeration describes the purpose for which the statistics is gathered.
    // if it's gathered to be dumped, then some different rules may be applied
    enum MemStatsPurposeEnum
    {
        nMSP_ForDisplay, nMSP_ForDump, nMSP_ForCrashLog, nMSP_ForBudget
    };
    // collects the whole memory statistics into the given sizer object
    void CollectMemStats (class ICrySizer* pSizer, MemStatsPurposeEnum nPurpose = nMSP_ForDisplay, std::vector<SmallModuleInfo>* pStats = 0);
    void GetExeSizes (ICrySizer* pSizer, MemStatsPurposeEnum nPurpose = nMSP_ForDisplay);
    //! refreshes the m_pMemStats if necessary; creates it if it's not created
    void TickMemStats(MemStatsPurposeEnum nPurpose = nMSP_ForDisplay, IResourceCollector* pResourceCollector = 0);

#if !defined(RELEASE)
    void SetVersionInfo(const char* const szVersion);
#endif

    virtual bool InitializeEngineModule(const char* dllName, const char* moduleClassName, const SSystemInitParams& initParams) override;
    virtual bool UnloadEngineModule(const char* dllName, const char* moduleClassName);
    virtual const IImageHandler* GetImageHandler() const override { return m_imageHandler.get(); }

    void ShutdownModuleLibraries();

#if defined(WIN32)
    friend LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif
    virtual void* GetRootWindowMessageHandler();
    virtual void RegisterWindowMessageHandler(IWindowMessageHandler* pHandler);
    virtual void UnregisterWindowMessageHandler(IWindowMessageHandler* pHandler);

    // IWindowMessageHandler
#if defined(WIN32)
    virtual bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
#endif
    // ~IWindowMessageHandler

private:

    // Release all resources.
    void    ShutDown();

    void SleepIfInactive();

    bool LoadEngineDLLs();

    //! @name Initialization routines
    //@{

    bool InitNetwork(const SSystemInitParams& startupParams);
    bool InitInput(const SSystemInitParams& startupParams);

    bool InitConsole();
    bool InitRenderer(WIN_HINSTANCE hinst, WIN_HWND hwnd, const SSystemInitParams& initParams);
    bool InitPhysics(const SSystemInitParams& initParams);
    bool InitPhysicsRenderer(const SSystemInitParams& initParams);

    bool InitFont(const SSystemInitParams& initParams);
    bool InitAISystem(const SSystemInitParams& initParams);
    bool InitScriptSystem(const SSystemInitParams& initParams);
    bool InitFileSystem(const SSystemInitParams& initParams);
    bool InitFileSystem_LoadEngineFolders(const SSystemInitParams& initParams);
    bool InitStreamEngine();
    bool Init3DEngine(const SSystemInitParams& initParams);
    bool InitAudioSystem(const SSystemInitParams& initParams);
    bool InitAnimationSystem(const SSystemInitParams& initParams);
    bool InitShine(const SSystemInitParams& initParams);
    bool InitEntitySystem(const SSystemInitParams& initParams);
    bool InitDynamicResponseSystem(const SSystemInitParams& initParams);
    bool OpenRenderLibrary(int type, const SSystemInitParams& initParams);
    bool InitGems(const SSystemInitParams& initParams);
    bool InitLmbrAWS(const SSystemInitParams& initParams);

#ifdef REMOTE_ASSET_PROCESSOR
    bool ConnectToAssetProcessor(const SSystemInitParams& initParams, bool waitTillConnect = false);
    void ConnectFromAssetProcessor(const SSystemInitParams& initParams, bool waitTillConnect = false);
    void WaitForAssetProcessorToBeReady();
#endif

    bool OpenRenderLibrary(const char* t_rend, const SSystemInitParams& initParams);

    //@}

    //////////////////////////////////////////////////////////////////////////
    // Threading functions.
    //////////////////////////////////////////////////////////////////////////
    void InitThreadSystem();
    void ShutDownThreadSystem();

    //////////////////////////////////////////////////////////////////////////
    // Helper functions.
    //////////////////////////////////////////////////////////////////////////
    void CreateRendererVars(const SSystemInitParams& startupParams);
    void CreateSystemVars();
    void CreateAudioVars();
    void RenderStats();
    void RenderOverscanBorders();
    void RenderMemStats();
    void RenderThreadInfo();

    AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadDLL(const char* dllName);

    void FreeLib(AZStd::unique_ptr<AZ::DynamicModuleHandle>& hLibModule);

    bool UnloadDLL(const char* dllName);
    void QueryVersionInfo();
    void LogVersion();
    void LogBuildInfo();
    void SetDevMode(bool bEnable);

    void CreatePhysicsThread();
    void KillPhysicsThread();

    static void TelemetryStreamFileChanged(ICVar* pCVar);
    static void TelemetryStreamIPChanged(ICVar* pCVar);

#ifndef _RELEASE
    static void SystemVersionChanged(ICVar* pCVar);
#endif // #ifndef _RELEASE

    bool ReLaunchMediaCenter();
    void LogSystemInfo();
    void UpdateAudioSystems();

    void AddCVarGroupDirectory(const string& sPath);

    AZStd::unique_ptr<AZ::DynamicModuleHandle> LoadDynamiclibrary(const char* dllName) const;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_3
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_h_provo.inl"
    #endif
#elif defined(WIN32)
    bool GetWinGameFolder(char* szMyDocumentsPath, int maxPathSize);
#endif

    //! \brief Initializes the given IFFont member variable with the given name (internal use only).
    bool LoadFontInternal(IFFont*& font, const string& fontName);

public:
    void EnableFloatExceptions(int type);

    // interface ISystem -------------------------------------------
    virtual IDataProbe* GetIDataProbe() { return m_pDataProbe; };
    virtual void SetForceNonDevMode(const bool bValue);
    virtual bool GetForceNonDevMode() const;
    virtual bool WasInDevMode() const { return m_bWasInDevMode; };
    virtual bool IsDevMode() const { return m_bInDevMode && !GetForceNonDevMode(); }
    virtual bool IsMinimalMode() const { return m_bMinimal; }
    virtual bool IsMODValid(const char* szMODName) const
    {
        if (!szMODName || strstr(szMODName, ".") || strstr(szMODName, "\\"))
        {
            return (false);
        }
        return (true);
    }
    virtual void AutoDetectSpec(const bool detectResolution);

    virtual void AsyncMemcpy(void* dst, const void* src, size_t size, int nFlags, volatile int* sync)
    {
#if !defined(CRY_ASYNC_MEMCPY_DELEGATE_TO_CRYSYSTEM)
        cryAsyncMemcpy(dst, src, size, nFlags, sync);
#else
        cryAsyncMemcpyDelegate(dst, src, size, nFlags, sync);
#endif
    }
    virtual void SetConsoleDrawEnabled(bool enabled) { m_bDrawConsole = enabled; }
    virtual void SetUIDrawEnabled(bool enabled) { m_bDrawUI = enabled; }

    // -------------------------------------------------------------

    //! attaches the given variable to the given container;
    //! recreates the variable if necessary
    ICVar* attachVariable (const char* szVarName, int* pContainer, const char* szComment, int dwFlags = 0);

    CCpuFeatures* GetCPUFeatures() { return m_pCpu; };

    string& GetDelayedScreeenshot() {return m_sDelayedScreeenshot; }

    CVisRegTest*& GetVisRegTestPtrRef() {return m_pVisRegTest; }

    const CTimeValue& GetLastTickTime(void) const { return m_lastTickTime; }
    const ICVar* GetDedicatedMaxRate(void) const { return m_svDedicatedMaxRate; }
   
    const char* GetRenderingDriverName(void) const
    {
        if(m_rDriver)
        {
            return m_rDriver->GetString();
        }
        return NULL;
    }

    
    std::shared_ptr<AZ::IO::FileIOBase> CreateLocalFileIO();

    // Gets the dimensions (in pixels) of the primary physical display.
    // Returns true if this info is available, returns false otherwise.
    bool GetPrimaryPhysicalDisplayDimensions(int& o_widthPixels, int& o_heightPixels);
    bool IsTablet();

private: // ------------------------------------------------------

    // System environment.
    SSystemGlobalEnvironment m_env;

    CTimer                              m_Time;                             //!<
    CCamera                             m_ViewCamera;                   //!<
    volatile bool                   m_bQuit;                            //!< if is true the system is quitting. Volatile as it may be polled from multiple threads.
    bool                                    m_bInitializedSuccessfully;     //!< true if the system completed all initialization steps
    bool                  m_bShaderCacheGenMode;//!< true if the application runs in shader cache generation mode
    bool                                    m_bRelaunch;                    //!< relaunching the app or not (true beforerelaunch)
    int                                     m_iLoadingMode;             //!< Game is loading w/o changing context (0 not, 1 quickloading, 2 full loading)
    bool                                    m_bTestMode;                    //!< If running in testing mode.
    bool                                    m_bMinimal;                     //!< If running in 'minimal mode'.
    bool                                    m_bEditor;                      //!< If running in Editor.
    bool                                    m_bNoCrashDialog;
    bool                  m_bPreviewMode;       //!< If running in Preview mode.
    bool                                    m_bDedicatedServer;     //!< If running as Dedicated server.
    bool                                    m_bIgnoreUpdates;           //!< When set to true will ignore Update and Render calls,
    IValidator*                    m_pValidator;                    //!< Pointer to validator interface.
    bool                                    m_bForceNonDevMode;     //!< true when running on a cheat protected server or a client that is connected to it (not used in singlplayer)
    bool                                    m_bWasInDevMode;            //!< Set to true if was in dev mode.
    bool                                    m_bInDevMode;                   //!< Set to true if was in dev mode.
    bool                  m_bGameFolderWritable;//!< True when verified that current game folder have write access.
    SDefaultValidator*     m_pDefaultValidator;     //!<
    string                              m_sDelayedScreeenshot;//!< to delay a screenshot call for a frame
    CCpuFeatures*                m_pCpu;                            //!< CPU features
    int                                     m_ttMemStatSS;              //!< Time to memstat screenshot
    string                m_szCmdLine;

    int                                     m_iTraceAllocations;

#ifndef _RELEASE
    int                                     m_checkpointLoadCount;// Total times game has loaded from a checkpoint
    LevelLoadOrigin             m_loadOrigin;                   // Where the load was initiated from
    bool                                    m_hasJustResumed;           // Has resume game just been called
    bool                                    m_expectingMapCommand;
#endif
    bool                                    m_bDrawConsole;              //!< Set to true if OK to draw the console.
    bool                                    m_bDrawUI;                   //!< Set to true if OK to draw UI.


    std::map<CCryNameCRC, AZStd::unique_ptr<AZ::DynamicModuleHandle> > m_moduleDLLHandles;

    //! THe streaming engine
    class CStreamEngine* m_pStreamEngine;

    //! current active process
    IProcess* m_pProcess;

    IMemoryManager* m_pMemoryManager;

    CPhysRenderer* m_pPhysRenderer;
    CCamera m_PhysRendererCamera;
    ICVar* m_p_draw_helpers_str;
    int m_iJumpToPhysProfileEnt;

    CTimeValue m_lastTickTime;

    //! system event dispatcher
    ISystemEventDispatcher* m_pSystemEventDispatcher;

    //! The default mono-spaced font for internal usage (profiling, debug info, etc.)
    IFFont* m_pIFont;

    //! The default font for end-user UI interfaces
    IFFont* m_pIFontUi;

    //! System to manage levels.
    ILevelSystem* m_pLevelSystem;

    //! System to manage views.
    IViewSystem* m_pViewSystem;

    //! System to monitor given budget.
    IBudgetingSystem* m_pIBudgetingSystem;

    //! System to access zlib compressor
    IZLibCompressor* m_pIZLibCompressor;

    //! System to access zlib decompressor
    IZLibDecompressor* m_pIZLibDecompressor;

    //! System to access lz4 hc decompressor
    ILZ4Decompressor* m_pILZ4Decompressor;

    //! System to load and access gems
    IGemManager* m_pGemManager;

    //! System for cryptography
    ICrypto* m_crypto;

    // XML Utils interface.
    class CXmlUtils* m_pXMLUtils;

    Serialization::IArchiveHost* m_pArchiveHost;

    int m_iApplicationInstance;

    //! to hold the values stored in system.cfg
    //! because editor uses it's own values,
    //! and then saves them to file, overwriting the user's resolution.
    int m_iHeight;
    int m_iWidth;
    int m_iColorBits;

    // System console variables.
    //////////////////////////////////////////////////////////////////////////

    // DLL names
    ICVar* m_sys_dll_response_system;
    ICVar* m_sys_dll_game;
    ICVar* m_sys_game_folder;
#if !defined(_RELEASE)
    ICVar* m_sys_resource_cache_folder;
#endif

#if AZ_LOADSCREENCOMPONENT_ENABLED
    ICVar* m_game_load_screen_uicanvas_path;
    ICVar* m_level_load_screen_uicanvas_path;
    ICVar* m_game_load_screen_sequence_to_auto_play;
    ICVar* m_level_load_screen_sequence_to_auto_play;
    ICVar* m_game_load_screen_sequence_fixed_fps;
    ICVar* m_level_load_screen_sequence_fixed_fps;
    ICVar* m_game_load_screen_max_fps;
    ICVar* m_level_load_screen_max_fps;
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED

    ICVar* m_sys_initpreloadpacks;
    ICVar* m_sys_menupreloadpacks;

    ICVar* m_cvAIUpdate;
    ICVar* m_rWidth;
    ICVar* m_rHeight;
    ICVar* m_rWidthAndHeightAsFractionOfScreenSize;
    ICVar* m_rTabletWidthAndHeightAsFractionOfScreenSize;
    ICVar* m_rHDRDolby;
    ICVar* m_rMaxWidth;
    ICVar* m_rMaxHeight;
    ICVar* m_rColorBits;
    ICVar* m_rDepthBits;
    ICVar* m_rStencilBits;
    ICVar* m_rFullscreen;
    ICVar* m_rFullscreenWindow;
    ICVar* m_rFullscreenNativeRes;
    ICVar* m_rDriver;
    ICVar* m_cvGameName;
    ICVar* m_rDisplayInfo;
    ICVar* m_rOverscanBordersDrawDebugView;
    ICVar* m_sysNoUpdate;
    ICVar* m_cvEntitySuppressionLevel;
    ICVar* m_pCVarQuit;
    ICVar* m_cvMemStats;
    ICVar* m_cvMemStatsThreshold;
    ICVar* m_cvMemStatsMaxDepth;
    ICVar* m_sysKeyboard;
    ICVar* m_sysWarnings;                                       //!< might be 0, "sys_warnings" - Treat warning as errors.
    ICVar* m_cvSSInfo;                                          //!< might be 0, "sys_SSInfo" 0/1 - get file sourcesafe info
    ICVar* m_svDedicatedMaxRate;
    ICVar* m_svAISystem;
    ICVar* m_clAISystem;
    ICVar* m_sys_profile;
    ICVar* m_sys_profile_additionalsub;
    ICVar* m_sys_profile_graph;
    ICVar* m_sys_profile_graphScale;
    ICVar* m_sys_profile_pagefaultsgraph;
    ICVar* m_sys_profile_filter;
    ICVar* m_sys_profile_filter_thread;
    ICVar* m_sys_profile_allThreads;
    ICVar* m_sys_profile_network;
    ICVar* m_sys_profile_peak;
    ICVar* m_sys_profile_peak_time;
    ICVar* m_sys_profile_memory;
    ICVar* m_sys_profile_sampler;
    ICVar* m_sys_profile_sampler_max_samples;
    ICVar* m_sys_GraphicsQuality;
    ICVar* m_sys_firstlaunch;
    ICVar* m_sys_skip_input;
    ICVar* m_sys_asset_processor;

    ICVar* m_sys_physics_CPU;

#if defined(AZ_RESTRICTED_PLATFORM)
#define AZ_RESTRICTED_SECTION SYSTEM_H_SECTION_4
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/System_h_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/System_h_provo.inl"
    #endif
#endif

    ICVar* m_sys_audio_disable;

    ICVar* m_sys_SimulateTask;

    ICVar* m_sys_min_step;
    ICVar* m_sys_max_step;
    ICVar* m_sys_enable_budgetmonitoring;
    ICVar* m_sys_memory_debug;
    ICVar* m_sys_preload;

    //  ICVar *m_sys_filecache;
    ICVar* m_gpu_particle_physics;

    string  m_sSavedRDriver;                                //!< to restore the driver when quitting the dedicated server

    //////////////////////////////////////////////////////////////////////////
    //! User define callback for system events.
    ISystemUserCallback* m_pUserCallback;

#if defined(CVARS_WHITELIST)
    //////////////////////////////////////////////////////////////////////////
    //! User define callback for whitelisting cvars
    ICVarsWhitelist* m_pCVarsWhitelist;
    ILoadConfigurationEntrySink* m_pCVarsWhitelistConfigSink;
#endif // defined(CVARS_WHITELIST)

    WIN_HWND        m_hWnd;
    WIN_HINSTANCE   m_hInst;

    // this is the memory statistics that is retained in memory between frames
    // in which it's not gathered
    class CrySizerStats* m_pMemStats;
    class CrySizerImpl* m_pSizer;

    CFrameProfileSystem m_FrameProfileSystem;

    class CThreadProfiler* m_pThreadProfiler;
    class IDiskProfiler*    m_pDiskProfiler;

    std::unique_ptr<IPlatformOS> m_pPlatformOS;
    ICryPerfHUD* m_pPerfHUD;
    minigui::IMiniGUI* m_pMiniGUI;

    //int m_nCurrentLogVerbosity;

    SFileVersion m_fileVersion;
    SFileVersion m_productVersion;
    SFileVersion m_buildVersion;
    IDataProbe* m_pDataProbe;

    class CLocalizedStringsManager* m_pLocalizationManager;

    // Name table.
    CNameTable m_nameTable;

    IThreadTask* m_PhysThread;

    // System telemetry streams
    Telemetry::CFileStream* m_pTelemetryFileStream;
    Telemetry::CUDPStream* m_pTelemetryUDPStream;

    ESystemConfigSpec m_nServerConfigSpec;
    ESystemConfigSpec m_nMaxConfigSpec;
    ESystemConfigPlatform m_ConfigPlatform;

    std::unique_ptr<CServerThrottle> m_pServerThrottle;

    CProfilingSystem m_ProfilingSystem;
    sUpdateTimes m_UpdateTimes[NUM_UPDATE_TIMES];
    uint32 m_UpdateTimesIdx;

    // Pause mode.
    bool m_bPaused;
    bool m_bNoUpdate;

    uint64 m_nUpdateCounter;

    int sys_ProfileLevelLoading, sys_ProfileLevelLoadingDump;

    bool m_executedCommandLine = false;

public:
    //! Pointer to the download manager
    class CDownloadManager* m_pDownloadManager;

    ICVar* m_sys_main_CPU;
    ICVar* m_sys_streaming_CPU;
    ICVar* m_sys_TaskThread_CPU[MAX_THREAD_POOL_INDEX];



#ifdef USE_FRAME_PROFILER
    void SetFrameProfiler(bool on, bool display, char* prefix) { m_FrameProfileSystem.SetProfiling(on, display, prefix, this); };
#else
    void SetFrameProfiler(bool on, bool display, char* prefix) {};
#endif

    //////////////////////////////////////////////////////////////////////////
    // File version.
    //////////////////////////////////////////////////////////////////////////
    virtual const SFileVersion& GetFileVersion();
    virtual const SFileVersion& GetProductVersion();
    virtual const SFileVersion& GetBuildVersion();

    bool CompressDataBlock(const void* input, size_t inputSize, void* output, size_t& outputSize, int level);
    bool DecompressDataBlock(const void* input, size_t inputSize, void* output, size_t& outputSize);

    bool InitVTuneProfiler();

    void OpenBasicPaks();
    void OpenLanguagePak(const char* sLanguage);
    void OpenLanguageAudioPak(const char* sLanguage);
    void GetLocalizedPath(const char* sLanguage, string& sLocalizedPath);
    void GetLocalizedAudioPath(const char* sLanguage, string& sLocalizedPath);
    void CloseLanguagePak(const char* sLanguage);
    void CloseLanguageAudioPak(const char* sLanguage);
    void UpdateMovieSystem(const int updateFlags, const float fFrameTime, const bool bPreUpdate);

    // level loading profiling
    virtual void OutputLoadingTimeStats();
    virtual struct SLoadingTimeContainer* StartLoadingSectionProfiling(CLoadingTimeProfiler* pProfiler, const char* szFuncName);
    virtual void EndLoadingSectionProfiling(CLoadingTimeProfiler* pProfiler);
    virtual const char* GetLoadingProfilerCallstack();

    //////////////////////////////////////////////////////////////////////////
    virtual CBootProfilerRecord* StartBootSectionProfiler(const char* name, const char* args);
    virtual void StopBootSectionProfiler(CBootProfilerRecord* record);
    virtual void StartBootProfilerSessionFrames(const char* pName);
    virtual void StopBootProfilerSessionFrames();

    //////////////////////////////////////////////////////////////////////////
    // CryAssert and error related.
    virtual bool RegisterErrorObserver(IErrorObserver* errorObserver);
    bool UnregisterErrorObserver(IErrorObserver* errorObserver);
    virtual void OnAssert(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber);
    void OnFatalError(const char* message);

    bool IsAssertDialogVisible() const;
    void SetAssertVisible(bool bAssertVisble);
    //////////////////////////////////////////////////////////////////////////

    virtual void ClearErrorMessages()
    {
        m_ErrorMessages.clear();
    }

    bool IsLoading()
    {
        return m_eRuntimeState == ESYSTEM_EVENT_LEVEL_LOAD_START_LOADINGSCREEN;
    }

    virtual ESystemGlobalState  GetSystemGlobalState(void);
    virtual void SetSystemGlobalState(const ESystemGlobalState systemGlobalState);

#if !defined(_RELEASE)
    virtual bool IsSavingResourceList() const { return (g_cvars.pakVars.nSaveLevelResourceList != 0); }
#endif

private:
    std::vector<IErrorObserver*> m_errorObservers;
    ESystemGlobalState m_systemGlobalState;
    static const char* GetSystemGlobalStateName(const ESystemGlobalState systemGlobalState);

public:
    void InitLocalization();
    void UpdateUpdateTimes();

protected: // -------------------------------------------------------------

    ILoadingProgressListener*      m_pProgressListener;
    CCmdLine*                                      m_pCmdLine;
    ITestSystem*                                   m_pTestSystem;               // needed for external test application (0 if not activated yet)
    CVisRegTest*                 m_pVisRegTest;
    CThreadManager*           m_pThreadManager;
    CThreadTaskManager*           m_pThreadTaskManager;
    class CResourceManager*       m_pResourceManager;
    ITextModeConsole*             m_pTextModeConsole;
    INotificationNetwork* m_pNotificationNetwork;

    string  m_binariesDir;
    string  m_currentLanguageAudio;
    string  m_assetPlatform; // ("es3" / "pc" / etc) describes the KIND of assets we load and controls where they're loaded from
    string  m_systemConfigName; // computed from system_(hardwareplatform)_(assetsPlatform) - eg, system_android_es3.cfg or system_android_opengl.cfg or system_windows_pc.cfg

    // the following variables capture what was set up in the systeminitparams after/during InitFileSystem
    // They are not actually to be used except to establish aliases like @user@
    string m_userRootDir;
    string m_cacheDir;
    string m_logsDir;

    std::vector< std::pair<CTimeValue, float> > m_updateTimes;

    CMemoryFragmentationProfiler    m_MemoryFragmentationProfiler;

    struct SErrorMessage
    {
        string m_Message;
        float m_fTimeToShow;
        float m_Color[4];
        bool m_HardFailure;
    };
    typedef std::list<SErrorMessage> TErrorMessages;
    TErrorMessages m_ErrorMessages;
    bool m_bHasRenderedErrorMessage;
    bool m_bNeedDoWorkDuringOcclusionChecks;

    ESystemEvent m_eRuntimeState;
    bool m_bIsAsserting;

    friend struct SDefaultValidator;
    friend struct SCryEngineFoldersLoader;
    //  friend void ScreenshotCmd( IConsoleCmdArgs *pParams );

    bool m_bIsSteamInitialized;

    std::unique_ptr<IImageHandler> m_imageHandler;
    std::vector<IWindowMessageHandler*> m_windowMessageHandlers;
    bool m_initedOSAllocator = false;
    bool m_initedSysAllocator = false;

    AZStd::unique_ptr<ThermalInfoHandler> m_thermalInfoHandler;
};

/*extern static */ bool QueryModuleMemoryInfo(SCryEngineStatsModuleInfo& moduleInfo, int index);
