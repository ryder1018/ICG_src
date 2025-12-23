#include <bits/stdc++.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "header/cube.h"
#include "header/animated_model.h"
#include "header/shader.h"
#include "header/stb_image.h"

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);
unsigned int loadCubemap(std::vector<std::string> &mFileName);

struct material_t{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float gloss;
};

struct light_t{
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct camera_t{
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    glm::vec3 target;
    
    float yaw;
    float pitch;
    float radius;
    float minRadius;
    float maxRadius;
    float orbitRotateSpeed;
    float orbitZoomSpeed;
    float minOrbitPitch;
    float maxOrbitPitch;
    // auto-orbit settings
    bool enableAutoOrbit;
    float autoOrbitSpeed; // degrees per second
};

// settings
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

// cube map 
unsigned int cubemapTexture;
unsigned int cubemapTextureRyder; // [NEW] Second skybox
unsigned int cubemapVAO, cubemapVBO;

// fade overlay
shader_program_t* fadeShader = nullptr;
unsigned int fadeVAO, fadeVBO;
float fadeAlpha = 0.0f;
bool isFadingOut = false;
bool isFadingIn = false;
bool isFinaleMode = false; // [NEW] Triggers scene switch
bool enableCrowdGS = false; // [NEW] Toggle geometry shader effect in finale
bool enableCrowdGSPulse = false; // [NEW] Alternate geometry shader effect
const float FADE_SPEED = 1.0f;

// shader programs

// shader programs
int shaderProgramIndex = 0;
std::vector<shader_program_t*> shaderPrograms;
light_t light;
material_t material;
camera_t camera;

// --- Global Variables ---
// animated model
AnimatedModel* animatedModel;
AnimatedModel* dogModel;
AnimatedModel* bananaModel;
AnimatedModel* allosaurusModel;
AnimatedModel* gromitModel;
bool isExploded = false;
float explosionLevel = 0.0f; // 0.0 = Normal Dog, >0.0 = Exploding Dog
const float MAX_EXPLOSION = 7.0f;
const float EXPLOSION_SPEED = 2.0f;
glm::mat4 modelMatrix;

// Shaders
// std::vector<shader_program_t*> shaderPrograms; // Removed duplicate
shader_program_t* flairShader = nullptr; // Dedicated shader for Flair (Toon, No GS)
shader_program_t* flairShaderGS = nullptr; // [NEW] Flair shader with geometry effect
shader_program_t* flairShaderGSPulse = nullptr; // [NEW] Flair shader with pulse GS
shader_program_t* dogShader = nullptr;   // Dedicated shader for Dog (Metallic + GS)
// int shaderProgramIndex = 0; // Removed duplicate
shader_program_t* cubemapShader;

// animation timing
float currentTime = 0.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void model_setup(){
#if defined(__linux__) || defined(__APPLE__)
    std::string fbx_file = "../../src/asset/Dancing_Twerk.fbx";
    std::string texture_dir = "../../src/asset/texture/";
#else
    std::string fbx_file = "../../src/asset/Flair.fbx";
    std::string texture_dir = "../../src/asset/texture/";
#endif

    // Load the animated FBX model
    animatedModel = new AnimatedModel(fbx_file);
    // animatedModel->loadTexture(texture_dir + "Mei_TEX.png"); // Using existing texture
    
    // Load the DogBalloon model
    std::string dog_file = "../../src/asset/obj/DogBalloon/DogBalloon.obj";
    dogModel = new AnimatedModel(dog_file);

    // Create a 1x1 white texture for DogBalloon because shaders multiply texture color
    unsigned int whiteTexture;
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whiteData[] = { 255, 255, 255, 255 }; // RGBA, all white
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whiteData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    dogModel->texture = whiteTexture; // Assign to dogModel

    // Load the Banana dancing model
    std::string banana_file = "../../src/asset/BananaDancing.fbx";
    bananaModel = new AnimatedModel(banana_file);
    if (bananaModel->texture == 0) {
        bananaModel->loadTexture(texture_dir + "T_M_MED_Banana_Smooth_Body_D.tga");
    }

    // Load the Dancing Allosaurus model
    std::string allosaurus_file = "../../src/asset/Samba Dancing.fbx";
    allosaurusModel = new AnimatedModel(allosaurus_file);
    if (allosaurusModel->texture == 0) {
        allosaurusModel->loadTexture(texture_dir + "Allosaurus_colorize_d.png");
    }

    // Load the Gromit model
    std::string gromit_file = "../../src/asset/gromit doing the metro man dance.fbx";
    gromitModel = new AnimatedModel(gromit_file);
    if (gromitModel->texture == 0) {
        gromitModel->loadTexture(texture_dir + "Tex_0028_0_dds_Base_Color_image.png");
    }

    modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::scale(modelMatrix, glm::vec3(10.0f, 10.0f, 10.0f)); // Initial scale (will be overridden in render)
    modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.6f, 0.0f)); 
}

void updateCamera(){
    // Convert spherical orbit definition to cartesian camera position
    float yawRad = glm::radians(camera.yaw);
    float pitchRad = glm::radians(camera.pitch);
    float cosPitch = cos(pitchRad);

    camera.position.x = camera.target.x + camera.radius * cosPitch * cos(yawRad);
    camera.position.y = camera.target.y + camera.radius * sin(pitchRad);
    camera.position.z = camera.target.z + camera.radius * cosPitch * sin(yawRad);

    camera.front = glm::normalize(camera.target - camera.position);
    camera.right = glm::normalize(glm::cross(camera.front, camera.worldUp));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}

void applyOrbitDelta(float yawDelta, float pitchDelta, float radiusDelta) {
    camera.yaw += yawDelta;
    camera.pitch = glm::clamp(camera.pitch + pitchDelta, camera.minOrbitPitch, camera.maxOrbitPitch);
    camera.radius = glm::clamp(camera.radius + radiusDelta, camera.minRadius, camera.maxRadius);
    updateCamera();
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    glm::vec2 orbitInput(0.0f);
    float zoomInput = 0.0f;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        orbitInput.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        orbitInput.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        orbitInput.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        orbitInput.y -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        zoomInput -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        zoomInput += 1.0f;

    if (orbitInput.x != 0.0f || orbitInput.y != 0.0f || zoomInput != 0.0f) {
        float yawDelta = orbitInput.x * camera.orbitRotateSpeed * deltaTime;
        float pitchDelta = orbitInput.y * camera.orbitRotateSpeed * deltaTime;
        float radiusDelta = zoomInput * camera.orbitZoomSpeed * deltaTime;
        applyOrbitDelta(yawDelta, pitchDelta, radiusDelta);
    }
}

//////////////////////////////////////////////////////////////////////////
// Parameter setup, 
// You can change any of the settings if you want

void camera_setup(){
    camera.worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.yaw = 90.0f;
    camera.pitch = 10.0f;
    camera.radius = 400.0f;
    camera.minRadius = 40.0f; // [MODIFIED] Allow getting closer (was 150.0f)
    camera.maxRadius = 800.0f;
    camera.orbitRotateSpeed = 60.0f;
    camera.orbitZoomSpeed = 400.0f;
    camera.minOrbitPitch = -80.0f;
    camera.maxOrbitPitch = 80.0f;
    camera.target = glm::vec3(0.0f);
    camera.enableAutoOrbit = true;
    camera.autoOrbitSpeed = 20.0f;

    updateCamera();
}

void light_setup(){
    light.position = glm::vec3(1000.0, 1000.0, 0.0);
    light.ambient = glm::vec3(1.0);
    light.diffuse = glm::vec3(1.0);
    light.specular = glm::vec3(1.0);
}

void material_setup(){
    material.ambient = glm::vec3(0.5);
    material.diffuse = glm::vec3(1.0);
    material.specular = glm::vec3(0.7);
    material.gloss = 50.0;
}
//////////////////////////////////////////////////////////////////////////

void shader_setup(){
#if defined(__linux__) || defined(__APPLE__)
    std::string shaderDir = "../../src/shaders/";
#else
    std::string shaderDir = "../../src/shaders/";
#endif

    std::vector<std::string> shadingMethod = {
        "default", "bling-phong", "gouraud", "metallic", "glass_schlick", "toon"
    };

    // Create animated versions of all original shaders
    for(int i=0; i<shadingMethod.size(); i++){
        std::string vpath = shaderDir + "animated_" + shadingMethod[i] + ".vert";
        std::string fpath = shaderDir + shadingMethod[i] + ".frag";

        shader_program_t* shaderProgram = new shader_program_t();
        shaderProgram->create();
        shaderProgram->add_shader(vpath, GL_VERTEX_SHADER);
        shaderProgram->add_shader(fpath, GL_FRAGMENT_SHADER);
        
        // Add Explosion Geometry Shader (skip gouraud due to interface mismatch)
        if (shadingMethod[i] != "gouraud") {
            std::string gspath = shaderDir + "explosion.gs";
            shaderProgram->add_shader(gspath, GL_GEOMETRY_SHADER);
        }

        shaderProgram->link_shader();
        shaderPrograms.push_back(shaderProgram);
    }

    // --- Setup Flair Dedicated Shader (Toon, No GS) ---
    flairShader = new shader_program_t();
    flairShader->create();
    flairShader->add_shader(shaderDir + "animated_default.vert", GL_VERTEX_SHADER);
    flairShader->add_shader(shaderDir + "toon.frag", GL_FRAGMENT_SHADER);
    flairShader->link_shader();

    // --- Setup Flair Shader with Geometry Shader ---
    flairShaderGS = new shader_program_t();
    flairShaderGS->create();
    flairShaderGS->add_shader(shaderDir + "animated_default.vert", GL_VERTEX_SHADER);
    flairShaderGS->add_shader(shaderDir + "toon.frag", GL_FRAGMENT_SHADER);
    flairShaderGS->add_shader(shaderDir + "explosion.gs", GL_GEOMETRY_SHADER);
    flairShaderGS->link_shader();

    // --- Setup Flair Shader with Pulse Geometry Shader ---
    flairShaderGSPulse = new shader_program_t();
    flairShaderGSPulse->create();
    flairShaderGSPulse->add_shader(shaderDir + "animated_default.vert", GL_VERTEX_SHADER);
    flairShaderGSPulse->add_shader(shaderDir + "toon.frag", GL_FRAGMENT_SHADER);
    flairShaderGSPulse->add_shader(shaderDir + "pulse.gs", GL_GEOMETRY_SHADER);
    flairShaderGSPulse->link_shader();

    // --- Setup Dog Dedicated Shader (Metallic + Combined GS) ---
    dogShader = new shader_program_t();
    dogShader->create();
    // Re-use animated_metallic.vert (if it exists) or animated_default.vert
    // Usually metallic uses standard animated vert if no special needs. 
    // Let's assume animated_metallic.vert was used by original metallic setup if it exists, otherwise default.
    // In shader_setup loop: "animated_" + shadingMethod[i] + ".vert". Method is "metallic".
    // So "animated_metallic.vert" should exist.
    dogShader->add_shader(shaderDir + "animated_metallic.vert", GL_VERTEX_SHADER);
    dogShader->add_shader(shaderDir + "metallic.frag", GL_FRAGMENT_SHADER);
    // Use the UPDATED metallic.gs which now has explosion logic
    dogShader->add_shader(shaderDir + "metallic.gs", GL_GEOMETRY_SHADER);
    dogShader->link_shader();
}

void cubemap_setup(){
#if defined(__linux__) || defined(__APPLE__)
    std::string cubemapDir = "../../src/asset/texture/ablaze/";
    std::string texture_dir = "../../src/asset/texture/"; // [NEW] Base texture dir
    std::string shaderDir = "../../src/shaders/";
#else
    std::string cubemapDir = "../../src/asset/texture/ablaze/";
    std::string texture_dir = "../../src/asset/texture/"; // [NEW] Base texture dir
    std::string shaderDir = "../../src/shaders/";
#endif

    // setup texture for cubemap
    std::vector<std::string> faces
    {
        cubemapDir + "right.png",
        cubemapDir + "left.png",
        cubemapDir + "top.png",
        cubemapDir + "bottom.png",
        cubemapDir + "front.png",
        cubemapDir + "back.png"
    };
    cubemapTexture = loadCubemap(faces);   

    // setup shader for cubemap
    std::string vpath = shaderDir + "cubemap.vert";
    std::string fpath = shaderDir + "cubemap.frag";
    
    cubemapShader = new shader_program_t();
    cubemapShader->create();
    cubemapShader->add_shader(vpath, GL_VERTEX_SHADER);
    cubemapShader->add_shader(fpath, GL_FRAGMENT_SHADER);
    cubemapShader->link_shader();

    glGenVertexArrays(1, &cubemapVAO);
    glGenBuffers(1, &cubemapVBO);
    glBindVertexArray(cubemapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubemapVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubemapVertices), &cubemapVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // [NEW] Load Ryder Skybox
    std::vector<std::string> facesRyder
    {
        texture_dir + "ryder/right.png",
        texture_dir + "ryder/left.png",
        texture_dir + "ryder/top.png",
        texture_dir + "ryder/bottom.png",
        texture_dir + "ryder/front.png",
        texture_dir + "ryder/back.png"
    };
    cubemapTextureRyder = loadCubemap(facesRyder);
}

void fade_setup() {
    // [NEW] Setup Fade Shader and Quad
#if defined(__linux__) || defined(__APPLE__)
    std::string shaderDir = "../../src/shaders/";
#else
    std::string shaderDir = "../../src/shaders/";
#endif

    fadeShader = new shader_program_t();
    fadeShader->create();
    fadeShader->add_shader(shaderDir + "fade.vert", GL_VERTEX_SHADER);
    fadeShader->add_shader(shaderDir + "fade.frag", GL_FRAGMENT_SHADER);
    fadeShader->link_shader();

    float quadVertices[] = {
        // positions
        -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,

        -1.0f,  1.0f, 0.0f,
         1.0f, -1.0f, 0.0f,
         1.0f,  1.0f, 0.0f
    };

    glGenVertexArrays(1, &fadeVAO);
    glGenBuffers(1, &fadeVBO);
    glBindVertexArray(fadeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fadeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void renderAnimatedCharacter(shader_program_t* shader, AnimatedModel* model, const glm::mat4& modelMat) {
    shader->set_uniform_value("model", modelMat);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, model->texture);
    shader->set_uniform_value("ourTexture", 0);

    GLint boneMatricesLocation = glGetUniformLocation(shader->get_program_id(), "finalBonesMatrices");
    if (boneMatricesLocation != -1) {
        const std::vector<glm::mat4>& transforms = model->m_FinalBoneMatrices;
        if (!transforms.empty()) {
            glUniformMatrix4fv(boneMatricesLocation, transforms.size(), GL_FALSE, &transforms[0][0][0]);
        }
    }

    model->render();
}

void setup(){
    // initialize shader model camera light material
    light_setup();
    model_setup();
    shader_setup();
    camera_setup();
    cubemap_setup();
    fade_setup(); // [NEW]
    material_setup();

    // enable depth test, face culling
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);

    // debug - but filter out expected uniform location errors
    // glEnable(GL_DEBUG_OUTPUT);
    // glDebugMessageCallback([](  GLenum source, GLenum type, GLuint id, GLenum severity, 
    //                             GLsizei length, const GLchar* message, const void* userParam) {

    // std::cerr << "GL CALLBACK: " << (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "") 
    //           << "type = " << type 
    //           << ", severity = " << severity 
    //           << ", message = " << message << std::endl;
    // }, nullptr);
}

void update(){
    // Update timing
    currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;
    
    // Update animation
    animatedModel->updateAnimation(currentTime);
    bananaModel->updateAnimation(currentTime);
    allosaurusModel->updateAnimation(currentTime);
    gromitModel->updateAnimation(currentTime);

    // Auto-orbit camera around target
    if (camera.enableAutoOrbit) {
        // float yawDelta = camera.autoOrbitSpeed * deltaTime;
        float yawDelta = camera.autoOrbitSpeed * 0;
        applyOrbitDelta(yawDelta, 0.0f, 0.0f);
    }

    // [NEW] Logic: Camera Proximity Event
    // If not already in finale mode and not fading out, check distance.
    if (!isFinaleMode && !isFadingOut && !isFadingIn) {
        if (camera.radius < 100.0f) {
            isFadingOut = true; // Trigger Event
        }
    }

    // [NEW] Fade Animation Logic
    if (isFadingOut) {
        fadeAlpha += FADE_SPEED * deltaTime;
        if (fadeAlpha >= 1.0f) {
            fadeAlpha = 1.0f;
            isFinaleMode = true; // SWITCH SCENE
            isFadingOut = false;
            isFadingIn = true;   // Start Fading In
        }
    } else if (isFadingIn) {
        fadeAlpha -= FADE_SPEED * deltaTime;
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            isFadingIn = false;
        }
    }
}

void render(){
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // calculate view, projection matrix using new camera system
    glm::mat4 view = glm::lookAt(camera.position + glm::vec3(0.0f, -0.2f, -0.1f), camera.position + camera.front, camera.up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);


    
    // --- RENDER LOGIC START ---
    
    // 0. Render Skybox FIRST (Background)
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST); // Ensure skybox passes depth test (since it's background)
    glDisable(GL_CULL_FACE);  // Ensure inside faces are visible

    cubemapShader->use();
    // Remove translation from view matrix for skybox ensuring it stays centered
    glm::mat4 skyboxView = glm::mat4(glm::mat3(view)); 
    cubemapShader->set_uniform_value("view", skyboxView); 
    cubemapShader->set_uniform_value("projection", projection);
    cubemapShader->set_uniform_value("skybox", 0);
    glActiveTexture(GL_TEXTURE0);
    // [NEW] Switch Skybox Texture based on Mode
    if (isFinaleMode) {
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTextureRyder);
    } else {
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    }
    glBindVertexArray(cubemapVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    cubemapShader->release();
    
    glDepthMask(GL_TRUE); 
    glEnable(GL_DEPTH_TEST); // Restore for rest of scene

    // Visibility Logic
    // User Request: 
    // 1. "isExplode=True -> Immediately spawn Flair" -> Draw Flair if isExploded.
    // 2. "False -> Disappear later" -> Draw Flair while explosionLevel > 0.
    // 3. "Explosion前后都能看到Flair" -> Overlap allowed.
    
    // 3. "Explosion前后都能看到Flair" -> Overlap allowed.
    
    // [MODIFIED] Finale Mode: Only Flair is visible (and Skybox)
    bool drawFlair = isFinaleMode || (isExploded || (explosionLevel > 0.0f));
    
    // [MODIFIED] Hide Dog in Finale Mode
    bool drawDog = !isFinaleMode && (explosionLevel < MAX_EXPLOSION);

    // 1. Update Explosion Animation
    if (isExploded) {
        explosionLevel += deltaTime * EXPLOSION_SPEED;
        if (explosionLevel > MAX_EXPLOSION) explosionLevel = MAX_EXPLOSION;
    } else {
        explosionLevel -= deltaTime * EXPLOSION_SPEED;
        if (explosionLevel < 0.0f) explosionLevel = 0.0f;
    }

    // 2. Render Opaque Objects First (Flair + Finale Dancers)
    if (drawFlair) {
        shader_program_t* crowdShader = flairShader;
        float crowdMagnitude = 0.0f;
        if (isFinaleMode && enableCrowdGSPulse) {
            crowdShader = flairShaderGSPulse;
        } else if (isFinaleMode && enableCrowdGS) {
            crowdShader = flairShaderGS;
            crowdMagnitude = 0.25f + 0.1f * sin(currentTime * 3.0f);
        }

        crowdShader->use();

        crowdShader->set_uniform_value("view", view);
        crowdShader->set_uniform_value("projection", projection);
        crowdShader->set_uniform_value("viewPos", camera.position);
        crowdShader->set_uniform_value("magnitude", crowdMagnitude);
        crowdShader->set_uniform_value("time", currentTime);

        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND); // Solid
        glDepthMask(GL_TRUE); // Write Depth

        crowdShader->set_uniform_value("material.diffuse", material.diffuse);
        crowdShader->set_uniform_value("material.ambient", material.ambient);
        crowdShader->set_uniform_value("material.specular", material.specular);
        crowdShader->set_uniform_value("material.gloss", material.gloss);

        crowdShader->set_uniform_value("light.position", light.position);
        crowdShader->set_uniform_value("light.ambient", light.ambient);
        crowdShader->set_uniform_value("light.diffuse", light.diffuse);
        crowdShader->set_uniform_value("light.specular", light.specular);
        crowdShader->set_uniform_value("lightIntensity", 1.0f); 

        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));
        renderAnimatedCharacter(crowdShader, animatedModel, modelMatrix);

        if (isFinaleMode) {
            float bananaAngle = currentTime * 2.5f;
            float dinoAngle = bananaAngle + 3.1415926f;
            glm::vec3 orbitCenter(0.0f, -0.6f, 0.0f);

            // Banana: position/scale (finale crowd)
            glm::vec3 bananaPos(
                orbitCenter.x + cos(bananaAngle) * 50.0f,
                -0.8f,
                orbitCenter.z + sin(bananaAngle) * 50.0f
            );
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, bananaPos);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));
            renderAnimatedCharacter(crowdShader, bananaModel, modelMatrix);

            // Allosaurus: position/scale (finale crowd)
            glm::vec3 dinoPos(
                orbitCenter.x + cos(dinoAngle) * 36.0f,
                -1.2f,
                orbitCenter.z + sin(dinoAngle) * 36.0f
            );
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, dinoPos);
            modelMatrix = glm::scale(modelMatrix, glm::vec3(0.15f));
            renderAnimatedCharacter(crowdShader, allosaurusModel, modelMatrix);

            // Gromit: position/scale (finale crowd)
            modelMatrix = glm::mat4(1.0f);
            modelMatrix = glm::translate(modelMatrix, glm::vec3(10.0f, -0.8f, -57.0f));
            modelMatrix = glm::scale(modelMatrix, glm::vec3(0.8f));
            renderAnimatedCharacter(crowdShader, gromitModel, modelMatrix);
        }

        crowdShader->release();
    }

    // 3. Render Transparent/Blended Objects Second (Dog with Aura)
    if (drawDog) {
        // --- RENDER DOG (Metallic + Explosion GS + Aura) ---
        dogShader->use();
        dogShader->set_uniform_value("magnitude", explosionLevel); 
        dogShader->set_uniform_value("view", view);
        dogShader->set_uniform_value("projection", projection);
        dogShader->set_uniform_value("viewPos", camera.position);
        dogShader->set_uniform_value("time", currentTime);
        
        modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::scale(modelMatrix, glm::vec3(600.0f)); 
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.2f, 0.0f)); 
        dogShader->set_uniform_value("model", modelMatrix);

        // Alpha Blending for Aura!
        glDisable(GL_CULL_FACE); // See inside of explosion
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standard transparency
        glDepthMask(GL_TRUE); // Write Depth (Required for Solid Inner Layer)
        // Note: Transparent Aura will also write depth, but it's acceptable here.

        dogShader->set_uniform_value("material.diffuse", glm::vec3(0.012777f, 0.011200f, 0.800000f));
        dogShader->set_uniform_value("material.ambient", glm::vec3(0.012777f, 0.011200f, 0.800000f) * 0.5f); 
        dogShader->set_uniform_value("material.specular", glm::vec3(0.5f, 0.5f, 0.5f));
        dogShader->set_uniform_value("material.gloss", 64.0f); 

        dogShader->set_uniform_value("light.position", light.position);
        dogShader->set_uniform_value("light.ambient", light.ambient);
        dogShader->set_uniform_value("light.diffuse", light.diffuse);
        dogShader->set_uniform_value("light.specular", light.specular);
        dogShader->set_uniform_value("alpha", 0.4f); // Base alpha for metallic
        dogShader->set_uniform_value("lightIntensity", 1.0f);
        dogShader->set_uniform_value("bias", 0.2f); 

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        dogShader->set_uniform_value("skybox", 1);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dogModel->texture);
        dogShader->set_uniform_value("ourTexture", 0);

        dogModel->render();
        dogShader->release();
        
        // Restore Depth Mask
        glDepthMask(GL_TRUE);
        // Restore Depth Mask
        glDepthMask(GL_TRUE);
    }

    // [NEW] 4. Render Fade Overlay
    if (fadeAlpha > 0.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST); // Draw over everything
        
        fadeShader->use();
        fadeShader->set_uniform_value("alpha", fadeAlpha);
        
        glBindVertexArray(fadeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        fadeShader->release();
        
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
}

int main() {
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "HW3-Animated Model", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);
    glfwSwapInterval(1);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // set viewport
    glfwGetFramebufferSize(window, &SCR_WIDTH, &SCR_HEIGHT);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // setup texture, model, shader ...e.t.c
    setup();
    
    // render loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        update(); 
        render(); 
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // cleanup
    delete animatedModel;
    delete dogModel;
    delete bananaModel;
    delete allosaurusModel;
    delete gromitModel;
    delete flairShaderGS;
    delete flairShaderGSPulse;
    for (auto shader : shaderPrograms) {
        delete shader;
    }
    delete cubemapShader;

    glfwTerminate();
    return 0;
}

// add key callback
void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // shader program selection
    if (key == GLFW_KEY_0 && (action == GLFW_REPEAT || action == GLFW_PRESS)) 
        shaderProgramIndex = 0;
    if (key == GLFW_KEY_1 && (action == GLFW_REPEAT || action == GLFW_PRESS)) 
        shaderProgramIndex = 1;
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        enableCrowdGS = !enableCrowdGS;
        if (enableCrowdGS) enableCrowdGSPulse = false;
    }
    if (key == GLFW_KEY_2 && (action == GLFW_REPEAT || action == GLFW_PRESS)) 
        shaderProgramIndex = 2;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        enableCrowdGSPulse = !enableCrowdGSPulse;
        if (enableCrowdGSPulse) enableCrowdGS = false;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
        shaderProgramIndex = 3;
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
        shaderProgramIndex = 4;
    if (key == GLFW_KEY_5 && action == GLFW_PRESS)
        shaderProgramIndex = 5;
    // Clamp any higher keys to the last shader to avoid selecting an invalid index
    if (key == GLFW_KEY_6 && action == GLFW_PRESS)
        shaderProgramIndex = 5;
    if (key == GLFW_KEY_7 && action == GLFW_PRESS)
        shaderProgramIndex = 5;
    if (key == GLFW_KEY_8 && action == GLFW_PRESS)
        shaderProgramIndex = 5;

    // k key for explosion (switch model) toggle
    if (key == GLFW_KEY_K && action == GLFW_PRESS)
        isExploded = !isExploded;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

// loading cubemap texture
unsigned int loadCubemap(std::vector<std::string>& faces)
{
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        stbi_set_flip_vertically_on_load(false);
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 3);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
            );
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texture;
}
