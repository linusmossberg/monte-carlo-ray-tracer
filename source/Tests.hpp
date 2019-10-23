#pragma once

#include <iostream>
#include <fstream>
#include <string>

#ifdef _WIN32
    #include "windows.h"
    #include "psapi.h"
#endif

#include "PhotonMap.hpp"
#include "Scene.hpp"

void testPhotonMap(std::shared_ptr<Scene> scene);
