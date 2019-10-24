#include "main.hpp"

#include "SceneParser.hpp"
#include "Random.hpp"

int main()
{
    Random::seed(std::random_device{}());
    
    std::filesystem::path path(std::filesystem::current_path().string() + "\\scenes");
    std::cout << "Scene directory:" << std::endl << path.string() << std::endl << std::endl;

    std::vector<SceneOption> options;
    try
    {
        options = SceneParser::availible(path);
    }
    catch (const std::exception& ex)
    {
        std::cout << ex.what() << std::endl;
        waitForInput();
        return -1;
    }

    if (options.size() == 0)
    {
        std::cout << "No scenes found." << std::endl;
        waitForInput();
        return -1;
    }

    size_t scene_option = getSceneOption(options);
    {
        std::unique_ptr<SceneRenderer> scene_renderer;
        try
        {
            scene_renderer = std::make_unique<SceneRenderer>(SceneParser::parseScene(options[scene_option].path, options[scene_option].camera_idx));
        }
        catch (const std::exception& ex)
        {
            std::cout << ex.what() << std::endl;
            waitForInput();
            return -1;
        }

        scene_renderer->render();
    }

    waitForInput();
    
    return 0;
}