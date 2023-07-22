/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <iostream>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <dirent.h>

#include "esp_littlefs.h"
#include "wolfssl/ssl.h"


using namespace std;
bool InitializeFileSystem();
void DoNothing();

struct CSharedData
{
  TaskHandle_t Owner;
  
  WOLFSSL_CTX* m_pContext;
  
  WOLFSSL *m_pSession;
  
  CSharedData(TaskHandle_t hOwner)
  {
    Owner = hOwner;
    m_pContext = nullptr;
    m_pSession = nullptr;
  }
  
  ~CSharedData()
  {
    DestroySession();
    if (nullptr != m_pContext)
    {
      wolfSSL_CTX_free(m_pContext);
      m_pContext = nullptr;
    }
  }
  
  void CreateContext()
  {
    DestroyContext();

    cout << "Creating context on task " << pcTaskGetName(nullptr) << endl;
    WOLFSSL_METHOD* pMethod = wolfSSLv23_client_method();
    m_pContext = wolfSSL_CTX_new(pMethod);

    const char* pchCertificateAuthorityPath = "/content/CA-PubKey.crt";
    int nResult = wolfSSL_CTX_load_verify_locations(m_pContext, pchCertificateAuthorityPath, NULL);
    if (WOLFSSL_SUCCESS != nResult)
    {
      cerr << "Error " << nResult << " loading root authority from " << pchCertificateAuthorityPath << endl;
    }

    const char* pchClientCertificate = "/content/Device.crt";
    nResult = wolfSSL_CTX_use_certificate_file(m_pContext, pchClientCertificate, SSL_FILETYPE_PEM);
    if (WOLFSSL_SUCCESS != nResult)
    {
      cerr << "Error " << nResult << " loading client certificate from " << pchClientCertificate << endl;
    }
    
    const char* pchPrivateKey = "/content/D-Priv.der";
    nResult = wolfSSL_CTX_use_PrivateKey_file(m_pContext, pchPrivateKey, SSL_FILETYPE_ASN1);
    if (WOLFSSL_SUCCESS != nResult)
    {
      cerr << "Error " << nResult << " loading private key from " << pchPrivateKey << endl;
    }

  }

  void CreateSession()
  {
    DestroySession();
    cout << "Creating session on task " << pcTaskGetName(nullptr) << endl;
    m_pSession = wolfSSL_new(m_pContext);
  }
  
  void DestroySession()
  {
    if (nullptr != m_pSession)
    {
      cout << "Destroying session on task " << pcTaskGetName(nullptr) << endl;
      wolfSSL_Free(m_pSession);
      m_pSession = nullptr;
    }
  }
  
  void DestroyContext()
  {
    if (nullptr != m_pContext)
    {
      cout << "Destroying context on task " << pcTaskGetName(nullptr) << endl;
      wolfSSL_CTX_free(m_pContext);
    }
  }

};


void Task_CreateWolfSession(void* pArgument)
{
  CSharedData* pData = reinterpret_cast<CSharedData*>(pArgument);
  pData->CreateSession();
  cout << "Signalling\n";
  xTaskNotify(pData->Owner, 1, eSetValueWithOverwrite);
  
  DoNothing();
}

extern "C" void app_main(void)
{
  cout << "Wolf cross-task test\n";
  cout << "====================\n";
  
  InitializeFileSystem();
  
  CSharedData Data(xTaskGetCurrentTaskHandle());
  Data.CreateContext();
  TaskHandle_t hCreator;
  xTaskCreate(Task_CreateWolfSession, "Creator", 4096, &Data, 3, &hCreator);
  
  cout << "Waiting\n";

  uint32_t uValue;
  if (pdTRUE == xTaskNotifyWait(0, 0, &uValue, pdMS_TO_TICKS(100)))
  {
    cout << "Destroying session\n";
    Data.DestroySession();
  }
  else
  {
    cout << "Notification timeout\n";
  }
  
  DoNothing();
}



bool InitializeFileSystem()
{
  static esp_vfs_littlefs_conf_t Config =
  {
    .base_path = "/content",
    .partition_label = "content",
    .format_if_mount_failed = false, // should be prepared by uploader. 
    .dont_mount = false,
  };

  esp_err_t Result = esp_vfs_littlefs_register(&Config);
  if (ESP_OK != Result)
  {
    cerr << "Failed to mount file system: " << (int)Result << " => " << esp_err_to_name(Result) << endl;
    return false;
  }

  cout << "File system mounted. ";

  size_t szTotal, szUsed;
  Result = esp_littlefs_info(Config.partition_label, &szTotal, &szUsed);
  if (ESP_OK == Result)
  {
    cout << "Size = " << szTotal << ", used = " << szUsed << endl;
  }
  else
  {
    cerr << "Unable to retrieve partition information in " << __func__ << ". " << esp_err_to_name(Result) << endl;
    return false; 
  }

  DIR* pRoot;
  pRoot = opendir(Config.base_path);
  if (pRoot != nullptr)
  {
    cout << "File system content in " << Config.base_path << ':' << endl;
    dirent* pDirectory;
    while ((pDirectory = readdir(pRoot)) != nullptr)
    {
      cout << "  " << pDirectory->d_name << endl;
    }
    closedir(pRoot);
  }
  else
  {
    cout << "Directory not found\n";
  }
  cout << "-------\n\n";

  return true; 
}

void DoNothing()
{
  do
  {
    vTaskDelay(pdMS_TO_TICKS(100));
  } while (true);
}
