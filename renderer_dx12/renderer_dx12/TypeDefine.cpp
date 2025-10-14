#include "stdafx.h"

#include "TypeDefine.h"

void ResourceLoader::WCHARToString(WCHAR *wchar, std::string &s) {

  wchar_t *wText = wchar;
  DWORD length_of_wchar =
      WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);
  char *tem = new char[length_of_wchar];
  WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, tem, length_of_wchar, NULL,
                      FALSE);
  s = tem;
  delete[] tem;
}

// TODO: finish this function
void ResourceLoader::StringToWCHAR(std::string s, WCHAR *wchar) {

  // std::string temp = s;
  // int length_of_string = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(),
  // -1, NULL, 0); wchar_t * wszUtf8 = new wchar_t[length_of_string + 1];
  // memset(wszUtf8, 0, length_of_string * 2 + 2);
  // MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8,
  // length_of_string); memcpy(wchar, wszUtf8, length_of_string + 1); delete[]
  // wszUtf8;
}
