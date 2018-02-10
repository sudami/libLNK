#include "TestLNK.h"
#include "gtesthelper.h"

#include "libLNK.h"
#include "filesystemfunc.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#endif
#include <Windows.h>

gTestHelper & hlp = gTestHelper::getInstance();

std::string getExpectedFilePath()
{
  std::string file;
  file.append(hlp.getTestQualifiedName());
  file.append(".expected.txt");
  return file;
}

std::string getActualFilePath()
{
  std::string file;
  file.append(hlp.getTestQualifiedName());
  file.append(".actual.txt");
  return file;
}

bool createDummyFile(const char * iPath)
{
  FILE * f = fopen(iPath, "w");
  if (!f)
    return false;
  fputs("foobar", f);
  fclose(f);
  return true;
}

bool createDummyFile(const std::string & iPath, int size)
{
  FILE * f = fopen(iPath.c_str(), "w");
  if (!f)
    return false;
  static const int BUFFER_SIZE = 255;
  char buffer[BUFFER_SIZE];
  for(int i=0; i<BUFFER_SIZE; i++)
  {
    buffer[i] = (i*23+67)%(BUFFER_SIZE+1);
  }
  int numBlocks = size/BUFFER_SIZE;
  for(int i=0; i<numBlocks; i++)
  {
    fwrite(buffer, 1, BUFFER_SIZE, f);
  }
  fclose(f);
  return true;
}

bool deleteFile(const char * iPath)
{
  if (hlp.fileExists(iPath))
  {
    bool deleted = remove(iPath) == 0;
    return deleted;
  }
  return true; //file does not exists. Success.
}

struct LinkInfoDebug
{
  const char * target;
  const char * networkPath;
  const char * arguments;
  const char * description;
  const char * workingDirectory;
  const char * customIcon;
};

bool findAndCloseWindow(const char * iWindowTitle)
{
  //Detect the specified window (5 sec timeout)
  HWND hWnd = NULL;
  for(int i=0; i<100 && hWnd==NULL; i++)
  {
    Sleep(50);
    hWnd = FindWindow(NULL, iWindowTitle);
  }
  if (hWnd == NULL)
    return false; //window not found

  //Close the window
  SendMessage(hWnd, WM_CLOSE, 0, 0); //note: blocking call. use PostMessage for non-blocking call.

  bool success = false;
  for(int i=0; i<100 && success==false; i++)
  {
    Sleep(50);

    //find the window again, if not found, then close was successful
    hWnd = FindWindow(NULL, iWindowTitle);
    success = (hWnd == NULL);
  }

  return success;
}

void TestLNK::SetUp()
{

}

void TestLNK::TearDown()
{
}

TEST_F(TestLNK, testCreateSimpleLink)
{
  //test creation identical
  lnk::LinkInfo info;
  info.target = "C:\\Program Files (x86)\\PDFCreator\\History.txt";
  info.arguments = "\"this is the arguments\"";
  info.description = "this is my comment";
  info.workingDirectory = "C:\\Program Files (x86)\\PDFCreator";
  info.customIcon.filename = "%SystemRoot%\\system32\\SHELL32.dll";
  info.customIcon.index = 5;
  info.hotKey = lnk::LNK_NO_HOTKEY;

  const char * linkFilename = ".\\tests\\Simple.txt.copy.lnk";
  bool success = createLink(linkFilename, info);
  ASSERT_TRUE( success == true );

  //run the link
  std::string openCommandLine = "start \"\" ";
  openCommandLine += linkFilename;
  system(openCommandLine.c_str());
  ASSERT_TRUE( findAndCloseWindow("History.txt - Notepad") );

  //test command
  std::string command = lnk::getLinkCommand(linkFilename);
  std::string expectedCommand = info.target;
  expectedCommand += " ";
  expectedCommand += info.arguments;
  ASSERT_TRUE( command == expectedCommand );

  //test getLinkInfo on a custom (handmade) link
  {
    lnk::LinkInfo info;
    bool success = false;
    success = lnk::getLinkInfo(linkFilename, info);
    ASSERT_TRUE( success == true );
    LinkInfoDebug d = {
      info.target.c_str(),
      info.networkPath.c_str(),
      info.arguments.c_str(),
      info.description.c_str(),
      info.workingDirectory.c_str(),
      info.customIcon.filename.c_str(),
    };
    ASSERT_TRUE( info.target == "C:\\Program Files (x86)\\PDFCreator\\History.txt" );
    ASSERT_TRUE( info.networkPath == "" );
    ASSERT_TRUE( info.arguments == "\"this is the arguments\"" );
    ASSERT_TRUE( info.description == "this is my comment" );
    ASSERT_TRUE( info.workingDirectory == "C:\\Program Files (x86)\\PDFCreator" );
    ASSERT_TRUE( info.customIcon.filename == "%SystemRoot%\\system32\\SHELL32.dll" );
    ASSERT_TRUE( info.customIcon.index == 5 );
  }
}

TEST_F(TestLNK, testCreateCustomLink)
{
  //test creation
  lnk::LinkInfo info;
  info.target = "C:\\WINDOWS\\system32\\cmd.exe";
  info.arguments = "/c pause|echo this is a pause. please press a key";
  info.description = "testCreateCustomLink()";
  info.workingDirectory = filesystem::getCurrentFolder();
  info.workingDirectory += "\\tests";
  info.customIcon.filename = "C:\\Program Files (x86)\\PDFCreator\\PDFCreator.exe";
  info.customIcon.index = 0;
  info.hotKey = lnk::LNK_NO_HOTKEY;

  const char * linkFilename = "./tests/testcreate.copy.lnk";
  bool success = lnk::createLink(linkFilename, info);
  ASSERT_TRUE( success == true );

  //test command
  std::string command = lnk::getLinkCommand(linkFilename);
  std::string expectedCommand = info.target;
  expectedCommand += " ";
  expectedCommand += info.arguments;
  ASSERT_TRUE( command == expectedCommand );

  //test getLinkInfo on a custom (handmade) link
  {
    lnk::LinkInfo info;
    bool success = false;
    success = lnk::getLinkInfo(linkFilename, info);
    ASSERT_TRUE( success == true );
    LinkInfoDebug d = {
      info.target.c_str(),
      info.networkPath.c_str(),
      info.arguments.c_str(),
      info.description.c_str(),
      info.workingDirectory.c_str(),
      info.customIcon.filename.c_str(),
    };
    ASSERT_TRUE( info.target == "C:\\WINDOWS\\system32\\cmd.exe" );
    ASSERT_TRUE( info.networkPath == "" );
    ASSERT_TRUE( info.arguments == "/c pause|echo this is a pause. please press a key" );
    ASSERT_TRUE( info.description == "testCreateCustomLink()" );
    ASSERT_TRUE( info.workingDirectory == info.workingDirectory.c_str() );
    ASSERT_TRUE( info.customIcon.filename == "C:\\Program Files (x86)\\PDFCreator\\PDFCreator.exe" );
    ASSERT_TRUE( info.customIcon.index == 0 );
  }
}

TEST_F(TestLNK, testLinkInfo)
{
  {
    lnk::LinkInfo info;
    bool success = false;
    success = lnk::getLinkInfo("./tests/Simple.txt.lnk", info);
    ASSERT_TRUE( success == true );
    LinkInfoDebug d = {
      info.target.c_str(),
      info.networkPath.c_str(),
      info.arguments.c_str(),
      info.description.c_str(),
      info.workingDirectory.c_str(),
      info.customIcon.filename.c_str(),
    };
    ASSERT_TRUE( info.target == "C:\\Program Files\\PDFCreator\\History.txt" );
    ASSERT_TRUE( info.networkPath == "" );
    ASSERT_TRUE( info.arguments == "\"this is the arguments\"" );
    ASSERT_TRUE( info.description == "this is my comment" );
    ASSERT_TRUE( info.workingDirectory == "C:\\Program Files\\PDFCreator" );
    ASSERT_TRUE( info.customIcon.filename == "%SystemRoot%\\system32\\SHELL32.dll" );
    ASSERT_TRUE( info.customIcon.index == 5 );
  }

  {
    lnk::LinkInfo info;
    bool success = false;
    success = lnk::getLinkInfo("./tests/system.ini.lnk", info);
    ASSERT_TRUE( success == true );
    LinkInfoDebug d = {
      info.target.c_str(),
      info.networkPath.c_str(),
      info.arguments.c_str(),
      info.description.c_str(),
      info.workingDirectory.c_str(),
      info.customIcon.filename.c_str(),
    };
    ASSERT_TRUE( info.target == "C:\\WINDOWS\\system.ini" );
    ASSERT_TRUE( info.networkPath == "" );
    ASSERT_TRUE( info.arguments == "" );
    ASSERT_TRUE( info.description == "" );
    ASSERT_TRUE( info.workingDirectory == "C:\\WINDOWS" );
    ASSERT_TRUE( info.customIcon.filename == "" );
    ASSERT_TRUE( info.customIcon.index == 0 );
  }

  {
    lnk::LinkInfo info;
    bool success = false;
    success = lnk::getLinkInfo("./tests/usbdrive.txt.lnk", info);
    ASSERT_TRUE( success == true );
    LinkInfoDebug d = {
      info.target.c_str(),
      info.networkPath.c_str(),
      info.arguments.c_str(),
      info.description.c_str(),
      info.workingDirectory.c_str(),
      info.customIcon.filename.c_str(),
    };
    ASSERT_TRUE( info.target == "G:\\usbdrive.txt" );
    ASSERT_TRUE( info.networkPath == "" );
    ASSERT_TRUE( info.arguments == "" );
    ASSERT_TRUE( info.description == "" );
    ASSERT_TRUE( info.workingDirectory == "G:\\" );
    ASSERT_TRUE( info.customIcon.filename == "" );
    ASSERT_TRUE( info.customIcon.index == 0 );
  }
}
