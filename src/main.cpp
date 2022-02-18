#include <string>
#include <iostream>
#include <memory>
#include <filesystem>
#include "oglwindow.h"

namespace fs = std::filesystem;

// The OpenGL implementation in this program was inspired by the book:
//   "Learn OpenGL" by Joey de Vries - also available at learnopengl.com
//   The book explains much of how to use the following 3rd party libraries included here:
//     - GLFW (https://www.glfw.org/) - provides a simple API for creating windows,
//         contexts and surfaces, receiving input and events
//     - glad (https://github.com/Dav1dde/glad) - retrieves location of OS-specific
//         OpenGL functions and stores them in function pointers
//         - KHR (https://www.khronos.org/registry/EGL/api/KHR/khrplatform.h)
//     - glm (https://glm.g-truc.net/0.9.8/index.html) - a header only C++ mathematics
//         library for graphics software based on the OpenGL Shading Language (GLSL)
//         specifications.
//     - stb (https://github.com/nothings/stb/blob/master/stb_image.h) - image loading
//         library by Sean Barrett
//     - freetype (https://freetype.org/) - software development library that is able
//         to load fonts, render them to bitmaps, and provide support for several
//         font-related operations

int GetInput(std::string msg, int maxNum)
{
    //Check input and request revision if needed
    int rv;
    char *stopstring;
    std::string strTemp;

    std::cout << msg;
    std::cin >> strTemp;
	rv = std::stoi(strTemp, nullptr, 10);
    //Text input is read as zero
    while ((rv < 1) || (rv > maxNum))
    {
        std::cout << "That value does not correspond with one of the file number choices, please try again.\n";
        std::cin >> strTemp;
        rv = std::stoi(strTemp, nullptr, 10);
    }
    return rv;
}

std::string GetMeshFile()
{
    //List files in the ../res/stl folder and ask the user to select one by typing its associated number
	std::string rv;
    int fileNum;
    int i = 0;
    vector<std::string> meshFileNames;
    std::cout << " Available .stl (Stereolithography) Files" << std::endl;
    std::string path = "../res/stl";
    for (const auto & entry : fs::directory_iterator(path))
    {
        i++;
        meshFileNames.emplace_back(entry.path());
        std::cout << i << ". - " << entry.path() << std::endl;
    }
    //Check input and request revised if invalid 
    fileNum = GetInput("Number of File to be Displayed: ", i);
	std::cout << "Selected:  " << fileNum << "-" << meshFileNames[fileNum-1] << std::endl;
	rv = meshFileNames[fileNum-1];
	return rv;
}

int main()
{
    std::unique_ptr ufPath = std::make_unique<std::string>(GetMeshFile());
    OGLWindow oglwin;
    //Use move semantics to send filename to OGLWindow
	oglwin.InitOGLWindow(std::move(ufPath));

    return 0;
}

/*
#include <vector>
#include <thread>
#include <future>
#include "viewcontrols.h"

void OpenMultiWin(int thcount, int thmax, std::string mPath)
{
    std::unique_ptr ufPath = std::make_unique<std::string>(mPath);
    OGLWindow oglwin;
    oglwin.InitOGLWindow(thcount, thmax, std::move(ufPath));
}

int main()
{
    int thMax;
    std::string strTemp;
    std::cout << " This project can display up to three windows" << std::endl;
    std::cout << " If one window is selected, you can select the stl file to view" << std::endl;
    std::cout << " If 2 or 3 are selected, you will view a selection of stl files" << std::endl;
    std::cout << " that show the same object with different mesh refinement" << std::endl;
    thMax = GetInput("Enter the number of windows you would like to view (1, 2 or 3)", 3);
    if (thMax == 1)
    {
	    std::unique_ptr ufPath = std::make_unique<std::string>(GetMeshFile());
        OGLWindow oglwin;
	    oglwin.InitOGLWindow(0, 1, std::move(ufPath));
    }
    else
    {
        std::vector<std::string> multiPath;
        multiPath.emplace_back("../res/stl/425-0.50.stl");
        multiPath.emplace_back("../res/stl/425-0.25.stl");
        multiPath.emplace_back("../res/stl/425-0.10.stl");

        std::vector<std::future<void>> futures;

        // Open first OpenGL window in main thread
        // Open subsequent OpenGL windows in new threads
        //futures.emplace_back(std::async(std::launch::async, OpenMultiWin, 0, thMax, multiPath[0]));

        for (int thCount = 0; thCount < thMax; thCount++)
        {
	        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            futures.emplace_back(std::async(std::launch::async, OpenMultiWin, thCount, thMax, multiPath[thCount]));
        }
    }

    return 0;
}
*/
