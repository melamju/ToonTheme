#include "playground.h"

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Include GLM
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Include chrono for sleep command
#include <chrono>
#include <thread>

#include <common/shader.hpp>
#include "../../bin/parse_stl.h"

int main(void)
{
    //Initialize window
    bool windowInitialized = initializeWindow();
    if (!windowInitialized) return -1;

    //Initialize vertex buffer
    bool vertexbufferInitialized = initializeVertexbuffer();
    if (!vertexbufferInitialized) return -1;

    // Create and compile our GLSL program from the shaders
    programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");

    // Get a handle for our "MVP" and "MV" uniforms and update them for initalization 
    MatrixIDMV = glGetUniformLocation(programID, "MV");
    MatrixID = glGetUniformLocation(programID, "MVP");
    updateMVPTransformation();

    //initialize pose variables
    curr_x = 0;
    curr_y = -135;
    curr_angle = -20.15;

    //initialize daytime variables
    daytime = 90; //start of the day
    daysection = 0;
    lightPosX = -60.0;
    directionLight = glm::vec3(lightPosX, circleFunction(lightPosX, 90), 1.0);
    shininess = 10.0;
    materialColor = glm::vec3(0.752, 0.752, 0.752);
    saveMC = materialColor;

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    //start animation loop until escape key is pressed
    do {

        updateAnimationLoop();
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
        glfwWindowShouldClose(window) == 0);

    //Cleanup and close window
    cleanupVertexbuffer();
    glDeleteProgram(programID);
    closeWindow();

    return 0;
}

void updateAnimationLoop()
{
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use our shader
    glUseProgram(programID);

    // Update the variables for movement / rotation if a key was pressed
    if (glfwGetKey(window, GLFW_KEY_A)) { daycycle(-1); lightcycle(-1); } // -1 counterclockwise
    else if (glfwGetKey(window, GLFW_KEY_D)) { daycycle(1); lightcycle(1); } // 1 clockwise

    // Update the MVP transformation with the new values
    updateMVPTransformation();

    // Send our transformation to the currently bound shader, 
    // in the "MVP" uniform and also the "MV" uniform
    // Send custom light information
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(MatrixIDMV, 1, GL_FALSE, &MV[0][0]);
    glUniform3f(glGetUniformLocation(programID, "directionLight"), directionLight[0], directionLight[1], directionLight[2]); //send vec3
    glUniform3f(glGetUniformLocation(programID, "materialColor"), materialColor[0], materialColor[1], materialColor[2]); //send vec3
    glUniform1f(glGetUniformLocation(programID, "shininess"), shininess); //send float

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );

    // 2nd attribute buffer : normals
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);
    glVertexAttribPointer(
        1,                  // attribute 1. No particular reason for 1, but must match the layout in the shader.
        3,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        0,                  // stride
        (void*)0            // array buffer offset
    );

    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, vertexbuffer_size); // 3 indices starting at 0 -> 1 triangle

    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool initializeWindow()
{
    // Initialise GLFW
    if (!glfwInit())
    {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return false;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "Toon Theme", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return false;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(0.45f, 0.8f, 1.0f, 1.0f);
    return true;
}

bool updateMVPTransformation()
{
    // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 10000.0f);

    // Camera matrix (modify this to let the camera move)
    glm::mat4 View = glm::lookAt(
        glm::vec3(0, 0, 330), // Camera is at (0,0,0), in World Space
        glm::vec3(0, 0, 0), // and looks at the origin
        glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // Model matrix, example with 3 parameters (curr_angle, curr_x, curr_y)
    glm::mat4 Model = glm::mat4(1.0f); //start with identity matrix
    Model = glm::rotate(Model, curr_angle, glm::vec3(1.0f, 0.0f, 0.0f)); //apply orientation (last parameter: axis)
    Model = glm::translate(Model, glm::vec3(curr_x, curr_y, 0.0f)); //apply translation

    // Our ModelViewProjection : multiplication of our 3 matrices
    MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around
    MV = View * Model; //We also need MV in the shader to transform the light position

    return true;
}

bool initializeVertexbuffer()
{
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    //create vertex and normal data
    std::vector< glm::vec3 > vertices = std::vector< glm::vec3 >();
    std::vector< glm::vec3 > normals = std::vector< glm::vec3 >();
    loadSTLFile(vertices, normals, "Nier_Automata_full2.stl");
    vertexbuffer_size = vertices.size() * sizeof(glm::vec3);

    glGenBuffers(2, vertexbuffer); //generate two buffers, one for the vertices, one for the normals

    //fill first buffer (vertices)
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    //fill second buffer (normals)
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    return true;
}

bool cleanupVertexbuffer()
{
    // Cleanup VBO
    glDeleteVertexArrays(1, &VertexArrayID);
    return true;
}

bool closeWindow()
{
    glfwTerminate();
    return true;
}

void loadSTLFile(std::vector< glm::vec3 >& vertices,
    std::vector< glm::vec3 >& normals,
    std::string stl_file_name)
{
    stl::stl_data info = stl::parse_stl(stl_file_name);
    std::vector<stl::triangle> triangles = info.triangles;
    for (int i = 0; i < info.triangles.size(); i++) {
        stl::triangle t = info.triangles.at(i);
        glm::vec3 triangleNormal = glm::vec3(t.normal.x,
            t.normal.y,
            t.normal.z);
        //add vertex and normal for point 1:
        vertices.push_back(glm::vec3(t.v1.x, t.v1.y, t.v1.z));
        normals.push_back(triangleNormal);
        //add vertex and normal for point 2:
        vertices.push_back(glm::vec3(t.v2.x, t.v2.y, t.v2.z));
        normals.push_back(triangleNormal);
        //add vertex and normal for point 3:
        vertices.push_back(glm::vec3(t.v3.x, t.v3.y, t.v3.z));
        normals.push_back(triangleNormal);
    }
}

void daycycle(int direction) {

    if (daytime < 0) {
        daytime = 359;
    }

    if (daytime < 30) { //night
        glClearColor(0.08, 0.05, 0.25, 1.0);
        materialColor = saveMC + glm::vec3(-0.4, -0.4, -0.3);

        daytime += direction;
        directionChecker(direction);
    }
    else if (daytime < 90) { //night to day
        if (daysection >= 60) { daysection = 0; }
        if (daysection < 0) { daysection = 59; } //only want to interpolate in this section of the daytime
        float nDaysection = daysection / 60; //normalize to get a interpolation ratio

        //interpolate background
        glm::vec4 c1 = vec4(0.08f, 0.05f, 0.25f, 1.0f); //nightblue to
        glm::vec4 c2 = vec4(0.45f, 0.8f, 1.0f, 1.0f); //skyblue
        glm::vec4 c3 = interpolateLinear(c1, c2, nDaysection);
        glClearColor(c3[0], c3[1], c3[2], c3[3]);

        //interpolate materialColor
        c1 = vec4((saveMC + glm::vec3(-0.4, -0.4, -0.3)), 1.0f); //materialColor with darkblue touch
        c2 = vec4(saveMC, 1.0); //to materialColor
        c3 = interpolateLinearRGB(c1, c2, nDaysection);
        materialColor = glm::vec3(c3[0], c3[1], c3[2]);

        daytime += direction;
        daysection += direction;
    }
    else if (daytime < 210) { // day
        glClearColor(0.45, 0.8, 1.0, 1.0); //skyblue
        materialColor = saveMC;
        daytime += direction;
        daysection = 0;
        directionChecker(direction);
    }
    else if (daytime < 270) { //day to night
    
        if (daysection >= 60) { daysection = 0; }
        if (daysection < 0) { daysection = 59; } //only want to interpolate in this section of the daytime
        float nDaysection = daysection / 60; //normalize to get a interpolation ratio

        //interpolate background
        glm::vec4 c1 = vec4(0.45, 0.8, 1.0, 1.0); //skyblue to
        glm::vec4 c2 = vec4(0.08f, 0.05f, 0.25f, 1.0f); //nightblue
        glm::vec4 c3 = interpolateLinear(c1, c2, nDaysection);
        glClearColor(c3[0], c3[1], c3[2], c3[3]);

        //interpolate materialColor
        c1 = vec4(saveMC, 1.0); //materialColor to
        c2 = vec4((saveMC + glm::vec3(-0.4, -0.4, -0.3)), 1.0f); //materialColor with darkblue touch
        c3 = interpolateLinearRGB(c1, c2, nDaysection);
        materialColor = glm::vec3(c3[0], c3[1], c3[2]);

        daytime += direction;
        daysection += direction;
    }
    else if (daytime < 360) { //night
        glClearColor(0.08, 0.05, 0.25, 1.0);
        materialColor = saveMC + glm::vec3(-0.4, -0.4, -0.3);

        daytime += direction;
        directionChecker(direction);
    }
    else if (daytime >= 360) {
        daytime = 0;
    }
    
}

glm::vec4 rgb2hsl(glm::vec4 color) {
    //https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion

    float r = color[0];
    float g = color[1];
    float b = color[2];

    float max = (r > g && r > b) ? r : (g > b) ? g : b;
    float min = (r < g && r < b) ? r : (g < b) ? g : b;

    float h, s, l;
    l = (max + min) / 2.0f;

    if (max == min) {
        h = s = 0.0f;
    }
    else {
        float d = max - min;
        s = (l > 0.5f) ? d / (2.0f - max - min) : d / (max + min);

        if (r > g && r > b)
            h = (g - b) / d + (g < b ? 6.0f : 0.0f);

        else if (g > b)
            h = (b - r) / d + 2.0f;

        else
            h = (r - g) / d + 4.0f;

        h /= 6.0f;
    }

    color = glm::vec4(h, s, l, 1.0f);
    return color;
}

glm::vec4 hsl2rgb(glm::vec4 color) {
    //https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
    
    float r, g, b;
    float h = color[0];
    float s = color[1];
    float l = color[2];

    if (s == 0.0f) {
        r = g = b = l; // achromatic
    }
    else {
        float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
        float p = 2 * l - q;
        r = hsl2rgbHelper(p, q, h + 1.0f / 3.0f);
        g = hsl2rgbHelper(p, q, h);
        b = hsl2rgbHelper(p, q, h - 1.0f / 3.0f);
    }

    color = glm::vec4(r, g, b, 1.0f);
    return color;
}

float hsl2rgbHelper(float p, float q, float t) {
    //https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion

    if (t < 0.0f) {
        t += 1.0f;
    }
    if (t > 1.0f) {
        t -= 1.0f;
    }
    if (t < 1.0f / 6.0f) {
        return p + (q - p) * 6.0f * t;
    }
    if (t < 1.0f / 2.0f) {
        return q;
    }
    if (t < 2.0f / 3.0f) {
        return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    }
    return p;
}

//https://stackoverflow.com/questions/13488957/interpolate-from-one-color-to-another
glm::vec4 interpolateLinear(glm::vec4 color1, glm::vec4 color2, float ratio) {
    
    color1 = rgb2hsl(color1);
    color2 = rgb2hsl(color2);
    glm::vec4 res;

    for (int i = 0; i < 3; i++) {
        res[i] = color1[i] * (1 - ratio) + color2[i] * ratio;
    }

    res = hsl2rgb(res);
    return res;
}

glm::vec4 interpolateLinearRGB(glm::vec4 color1, glm::vec4 color2, float ratio) {
    glm::vec4 res;

    for (int i = 0; i < 3; i++) {
        res[i] = color1[i] * (1 - ratio) + color2[i] * ratio;
    }

    return res;
}

float circleFunction(float x, int radius) {
    return abs(sqrt(pow(radius, 2) - pow(x, 2)));
}

void lightcycle(int direction) {
    if (lightPosX < -90) { // change to sun/moon
        lightPosX = 90.00;
    }
    if (lightPosX > 90) { 
        lightPosX = -90.00;
    }
    directionLight = glm::vec3(lightPosX, circleFunction(lightPosX, 90), 1.0);
    lightPosX += direction;

    //simulating more realistical sun-/moonset trough higher shininess => smaller specular light dot => weaker sun/moon
    if (direction == 1) {
        if (lightPosX <= -89) shininess = 70.0;

        if (lightPosX < -60 && lightPosX > -92) {
            shininess -= direction * 2;
        }
        else if (lightPosX > 60 && lightPosX < 92) {
            shininess += direction * 2;
        }
        else {
            shininess = 10.0;
        }
    } 
    else if (direction == -1){
        if (lightPosX >= 89) shininess = 70.0;

        if (lightPosX > 60 && lightPosX < 92) {
            shininess -= abs(direction) * 2;
        }
        else if (lightPosX < -60 && lightPosX > -92) {
            shininess += abs(direction) * 2;
        }
        else {
            shininess = 10.0;
        }
    }

}

void directionChecker(int direction){
    //for daysection
    if (direction == -1) {
        daysection = 59;
    }
    else {
        daysection = 0;
    }
}