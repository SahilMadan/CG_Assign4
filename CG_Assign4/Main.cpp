#include "GLHeaders.hpp"
#include "GLShaderLoader.hpp"
#include "Camera.hpp"
#include "ModelData.hpp"
#include "Shapes.hpp"
#include "Renderer.hpp"
#include "City.hpp"

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

#define TAU (6.283185307179586f)
#define DEG2RAD(x) ((x) / 360.0f * TAU)

#define SKY_DIR (glm::vec3(0, 1, 0))
#define FORWARD_DIR (glm::vec3(0, 0, 1))
#define ORIGIN  (glm::vec3(0))

static ModelData* terrainModel;
static ModelData* buildingModel;
static City* city;

static Renderer* renderer;
static Camera* cam1;

static float screenWidth = 800;
static float screenHeight = 600;

static long prevTime = 0;

// A simple structure for storing relevant information required for mouse control
static struct MouseHandler {
    MouseHandler() : x(0), y(0), prevX(0), prevY(0), isDown(false) {}
    int x, y;
    int prevX, prevY;
    bool isDown;
    inline int dx() { return x - prevX; }
    inline int dy() { return y - prevY; }
    inline void update(int nx, int ny) { prevX = x; prevY = y; x = nx; y = ny; }
} mouseHandler;


// Generates a square terrain with the specified texture loaded from a file
RawModelData genTerrainModel(const std::string& terrainTexture) {
    RawModelData data;
    RawModelData::Shape shape = shapes::quad(glm::vec3(-1, 0, 1), glm::vec3(-1, 0, -1),
        glm::vec3(1, 0, -1), glm::vec3(1, 0, 1));
    shape.textureName = terrainTexture;
    data.shapes.push_back(shape);
    return data;
}


// FIXME: This should be replaced by a better function for generating buildings
//  - Need to generate normals correctly
//  - Need to generate textures for day and night
//  - Need to generate bump map / heightmap
RawModelData genCube(const std::string& texture) {
    RawModelData data;
    RawModelData::Shape shape;
    // Top
    shape = shapes::quad(glm::vec3(-1, 1, 1), glm::vec3(-1, 1, -1),
        glm::vec3(1, 1, -1), glm::vec3(1, 1, 1));
    shape.textureName = texture;
    data.shapes.push_back(shape);

    // Bottom
    shape = shapes::quad(glm::vec3(-1, -1, 1), glm::vec3(-1, -1, -1),
        glm::vec3(1, -1, -1), glm::vec3(1, -1, 1));
    shape.textureName = texture;
    data.shapes.push_back(shape);

    // Left
    shape = shapes::quad(glm::vec3(-1, 1, 1), glm::vec3(-1, -1, 1),
        glm::vec3(-1, -1, -1), glm::vec3(-1, 1, -1));
    shape.textureName = texture;
    data.shapes.push_back(shape);

    // Right
    shape = shapes::quad(glm::vec3(1, 1, -1), glm::vec3(1, -1, -1),
        glm::vec3(1, -1, 1), glm::vec3(1, 1, 1));
    shape.textureName = texture;
    data.shapes.push_back(shape);

    // Front
    shape = shapes::quad(glm::vec3(-1, 1, -1), glm::vec3(-1, -1, -1),
        glm::vec3(1, -1, -1), glm::vec3(1, 1, -1));
    shape.textureName = texture;
    data.shapes.push_back(shape);

    // Back
    shape = shapes::quad(glm::vec3(1, 1, 1), glm::vec3(1, -1, 1),
        glm::vec3(-1, -1, 1), glm::vec3(-1, 1, 1));
    shape.textureName = texture;
    data.shapes.push_back(shape);


    return data;
}

// Initialise the program resources
void initResources() {
    GLuint program = initProgram(shaderFromFile("shaders/vshader.glsl", GL_VERTEX_SHADER),
        shaderFromFile("shaders/fshader.glsl", GL_FRAGMENT_SHADER));
    glUseProgram(program);

    cam1 = new Camera(glm::vec3(0.0f, 10.0f, 0.0f), ORIGIN);
    glm::mat4 proj = glm::perspective(DEG2RAD(60.0f), screenWidth / screenHeight, 0.1f, 200.0f);

    renderer = new Renderer(program, proj, cam1);

    buildingModel = new ModelData(genCube("data/default.tga"), renderer);
    terrainModel = new ModelData(genTerrainModel("data/default.tga"), renderer);

    city = new City(buildingModel);
}

// Display callback
void onDisplay() {
    glClearColor(0.7f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    city->draw(renderer);
    renderer->drawModel(terrainModel, ORIGIN, glm::vec3(40, 1, 40));

    // Swap buffers
    glutSwapBuffers();
}

// Idle callback
void onIdle() {
    long time = glutGet(GLUT_ELAPSED_TIME);
    float dt = static_cast<float>(time - prevTime) / 1000.0f;
    prevTime = time;

    glutPostRedisplay();
}

// Keyboard callback
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
    case 033: exit(EXIT_SUCCESS); break;
    case 'w': cam1->move(glm::vec3(0, 0, 0.2f)); break;
    case 's': cam1->move(glm::vec3(0, 0, -0.2f)); break;
    case 'a': cam1->move(glm::vec3(-0.2f, 0, 0)); break;
    case 'd': cam1->move(glm::vec3(0.2f, 0, 0)); break;
    }
}

// Mouse click callback
void onMouse(int button, int state, int x, int y) {
    mouseHandler.update(x, y);
}

// Mouse motion callback
void onMotion(int x, int y) {
    mouseHandler.update(x, y);
    // Compute the relative mouse movement
    const glm::vec3 movement = glm::vec3(TAU * static_cast<float>(mouseHandler.dx()) / screenWidth,
        TAU * static_cast<float>(-mouseHandler.dy()) / screenHeight, 0.0f);
    cam1->moveTarget(movement);

    glutPostRedisplay();
}

// Window shape callback
void onReshape(int width, int height) {
    screenWidth = static_cast<float>(width);
    screenHeight = static_cast<float>(height);
    glViewport(0, 0, width, height);

    renderer->proj = glm::perspective(DEG2RAD(60.0f), screenWidth / screenHeight, 0.1f, 200.0f);
}

// Program entry point
int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Assignment 4");

#ifndef __APPLE__
    glewInit();
#endif

    // Enable GL properties
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glCullFace(GL_TRUE);

    // Set up callbacks
    glutDisplayFunc(onDisplay);
    glutIdleFunc(onIdle);
    glutReshapeFunc(onReshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(onMouse);
    glutMotionFunc(onMotion);

    initResources();

    glutMainLoop();
    return 0;
}
