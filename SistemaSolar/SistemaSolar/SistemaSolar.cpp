#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream> 
#include <vector>
using namespace std;
#include <math.h>
#define _USE_MATH_DEFINES
#include <stb_image.h>
#include <shader_m.h>
#include <camera.h>
#include <model.h>

#define BUFFER_OFFSET(offset) ((void*)(offset))
#define MEMBER_OFFSET(s,m) ((char*)NULL + (offsetof(s,m)))

// settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// camera
Camera camera(glm::vec3(0.0f, 60.0f, -110.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// variaveis planetas
glm::vec3 posicao_centro = glm::vec3(0.0f, 0.0f, 0.0f);
float plan_velocidade = 0.5f;
float planet_raio = 10.0f;
int planeta = 0;

// lighting
glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
float acc1 = 0.2f;
float acc2 = 0.5f;
float acc3 = 1.0f;

// texturas
constexpr auto sol_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_sun.jpg";
constexpr auto mercurio_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_mercury.jpg";
constexpr auto venus_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_venus_surface.jpg";
constexpr auto terra_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_earth.jpg";
constexpr auto lua_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_moon.jpg";
constexpr auto marte_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_mars.jpg";
constexpr auto jupiter_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_jupiter.jpg";
constexpr auto saturno_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_saturn.jpg";
constexpr auto urano_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_uranus.jpg";
constexpr auto neptuno_path = "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/2k_neptune.jpg";

// funções
unsigned int loadTexture(const char* path);
unsigned int loadCubemap(vector<string> faces);
GLuint SolidSphere(float radius, int slices, int stacks);
void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void printPlanetInfo();

struct Character {
    GLuint TextureID;   // ID  texture
    glm::ivec2 Size;    // Tamanho do glifo
    glm::ivec2 Bearing;  // Deslocamento da linha de base para a esquerda/topo do glifo
    GLuint Advance;    // Deslocamento para avançar para o próximo glifo
};
map<GLchar, Character> Characters;
unsigned int VAO, VBO;

struct PlanetInfo {
    string Nome;
    string Velocidade;
    string Massa;
    string Diametro;
};
PlanetInfo Info;

int main()
{
    //Propriedades da esfera, camadas longitudinais e transversais
    int slices = 32;
    int stacks = 32;
    int numIndicies = (slices * stacks + slices) * 6;

    //inicializar camera

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Sistema Solar", NULL, NULL);
    if (window == NULL)
    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);;
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ################################## Ir buscar os shaders ##################################
    Shader sphereShader("shaders/lighting_maps.glsl", "shaders/lighting_maps.frag");
    Shader lightsphereShader("shaders/light_cube.glsl", "shaders/light_cube.frag");
    Shader skyboxShader("shaders/skybox.glsl", "shaders/skybox.frag");

    // Parte da configuração
      // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // ########################## Inicializar SKYBOX ##########################
    float skyboxVertices[] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    // skybox buffers
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


    //---------------Carregamos aqui o VAO do tipo esfera  -----------
    // Base de todas as esferas menos da saturno
    GLuint vao = SolidSphere(planet_raio, 32, 32);


    // Modelo de saturno (Assimp Model)
    Model ourModel("C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/Saturno/teste.mtl");

    // guardar a posição dos planetas
    glm::vec3 posicaoPlanetas[8]{};

    // ############################### Carregar Texturar ###############################
    unsigned int sol_tex = loadTexture(sol_path);
    unsigned int mercurio_tex = loadTexture(mercurio_path);
    unsigned int venus_tex = loadTexture(venus_path);
    unsigned int terra_tex = loadTexture(terra_path);
    unsigned int lua_tex = loadTexture(lua_path);
    unsigned int marte_tex = loadTexture(marte_path);
    unsigned int jupiter_tex = loadTexture(jupiter_path);
    unsigned int saturno_tex = loadTexture(saturno_path);
    unsigned int urano_tex = loadTexture(urano_path);
    unsigned int neptuno_tex = loadTexture(neptuno_path);

    vector <string> faces{
        "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/Sky/right.jpg",
        "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/Sky/left.jpg",
        "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/Sky/top.jpg",
        "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/Sky/bottom.jpg",
        "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/Sky/front.jpg",
        "C:/Users/pmpb7/source/repos/SistemaSolar/SistemaSolar/Textures/Sky/back.jpg",
    };

    unsigned int cubemapTexture = loadCubemap(faces);

    // ############################### GERAR VETORES COM POSIÇÂO DE CADA ORBITA ###############################
    std::vector<float> orbVert;
    GLfloat xx;
    GLfloat zz;
    float angl;
    for (int i = 0; i < (planet_raio * 2 * 10); i++)
    {
        angl = (float)(M_PI / 2 - i * (M_PI / (planet_raio * 10)));
        xx = sin(angl) * planet_raio;
        zz = cos(angl) * planet_raio;
        orbVert.push_back(xx);
        orbVert.push_back(0.0f);
        orbVert.push_back(zz);
    }

    // ############################### VAO-VBO para as ORBITAS ###############################
    GLuint VBO_t, VAO_t;
    glGenVertexArrays(1, &VAO_t);
    glGenBuffers(1, &VBO_t);
    glBindVertexArray(VAO_t);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_t);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * orbVert.size(), orbVert.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // inicializar memoria
    for (int i = 0; i < 8; i++) {
        posicaoPlanetas[i] = posicao_centro;
    }
    // loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {

        // frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // renderizar
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ativar o shader qe queremos usar
        sphereShader.use();

        //propriedades do material
        sphereShader.setInt("material.diffuse", 0);
        sphereShader.setVec3("material.specular", 0.5f, 0.5f, 0.5f);
        sphereShader.setFloat("material.shininess", 64.0f);
        // luz propriadades
        sphereShader.setVec3("light.ambient", acc1, acc1, acc1);
        sphereShader.setVec3("light.diffuse", acc2, acc2, acc2);
        sphereShader.setVec3("light.specular", acc3, acc3, acc3);
        sphereShader.setVec3("light.position", glm::vec3(acc1, acc2, acc3));
        sphereShader.setVec3("viewPos", camera.Position);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        sphereShader.setMat4("projection", projection);
        glm::mat4 view = camera.GetViewMatrix();
        sphereShader.setMat4("view", view);



        // ########################################## Terra #########################################
        //Aqui especificamos que VAO queremos que esteja ativo para o render do respetivo planeta
        glBindVertexArray(vao);

        //especificamos que textura queremos que fique ativa
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terra_tex);

        //Shader ativado para o desenho da esfera
        sphereShader.use();



        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 moon;
        // posição da terra
        double xx = sin(glfwGetTime() * plan_velocidade * 0.75f) * planet_raio * 3.0f * 2.0f;
        double zz = cos(glfwGetTime() * plan_velocidade * 0.75f) * planet_raio * 3.0f * 2.0f;
        posicaoPlanetas[2] = glm::vec3(xx, 0, zz);

        model = glm::translate(model, glm::vec3(xx, 0.0f, zz));
        moon = model;
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.4, 0.4, 0.4));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));


        //###################################### LUA ######################################

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lua_tex);

        sphereShader.use();

        model = glm::mat4(1.0f);
        xx = sin(glfwGetTime() * plan_velocidade * 10.5f) * planet_raio * 0.5f * 2.0f;
        zz = cos(glfwGetTime() * plan_velocidade * 10.5f) * planet_raio * 0.5f * 2.0f;
        model = glm::translate(moon, glm::vec3(xx, 0.0f, zz));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.1, 0.1, 0.1));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));



        //###################################### SOL ######################################
        glBindVertexArray(vao);
        glBindTexture(GL_TEXTURE_2D, sol_tex);


        lightsphereShader.use();


        lightsphereShader.setMat4("projection", projection);
        lightsphereShader.setMat4("view", view);

        model = glm::mat4(1.0f);
        model = glm::translate(model, posicao_centro);
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        lightsphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));


        //###################################### MERCURIO ######################################

        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mercurio_tex);

        sphereShader.use();


        model = glm::mat4(1.0f);
        xx = sin(glfwGetTime() * plan_velocidade) * planet_raio * 1.f * 2.f;
        zz = cos(glfwGetTime() * plan_velocidade) * planet_raio * 1.f * 2.f;
        posicaoPlanetas[0] = glm::vec3(xx, 0, zz);
        model = glm::translate(model, glm::vec3(xx, 0.0f, zz));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.2, 0.2, 0.2));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));



        //###################################### VENUS ######################################

        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, venus_tex);

        sphereShader.use();

        model = glm::mat4(1.0f);
        xx = sin(glfwGetTime() * plan_velocidade * 0.8f) * planet_raio * 2.f * 2.f;
        zz = cos(glfwGetTime() * plan_velocidade * 0.8f) * planet_raio * 2.f * 2.f;
        posicaoPlanetas[1] = glm::vec3(xx, 0, zz);
        model = glm::translate(model, glm::vec3(xx, 0.0f, zz));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.35, 0.35, 0.35));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));


        //###################################### MARTE ######################################

        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, marte_tex);

        sphereShader.use();

        model = glm::mat4(1.0f);
        xx = sin(glfwGetTime() * plan_velocidade * 0.6f) * planet_raio * 4.f * 2.f;
        zz = cos(glfwGetTime() * plan_velocidade * 0.6f) * planet_raio * 4.f * 2.f;
        posicaoPlanetas[3] = glm::vec3(xx, 0, zz);

        model = glm::translate(model, glm::vec3(xx, 0.0f, zz));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.3, 0.3, 0.3));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));


        //###################################### JUPITER ######################################

        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, jupiter_tex);

        sphereShader.use();

        model = glm::mat4(1.0f);
        xx = sin(glfwGetTime() * plan_velocidade * 0.5f) * planet_raio * 5.f * 2.5f;
        zz = cos(glfwGetTime() * plan_velocidade * 0.5f) * planet_raio * 5.f * 2.5f;
        posicaoPlanetas[4] = glm::vec3(xx, 0, zz);
        model = glm::translate(model, glm::vec3(xx, 0.0f, zz));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.6, 0.6, 0.6));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));


        //###################################### SATURNO ######################################
        //glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, saturno_tex);

        sphereShader.use();

        model = glm::mat4(1.0f);
        xx = sin(glfwGetTime() * plan_velocidade * 0.46f) * planet_raio * 6.f * 2.5f;
        zz = cos(glfwGetTime() * plan_velocidade * 0.46f) * planet_raio * 6.f * 2.5f;
        posicaoPlanetas[5] = glm::vec3(xx, 0, zz);

        model = glm::translate(model, glm::vec3(xx, 0.0f, zz));
        //
        model = glm::scale(model, glm::vec3(0.02f, 0.02f, 0.02f));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));

        sphereShader.setMat4("model", model);
        ourModel.Draw(sphereShader);

        //###################################### URANO ######################################
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, urano_tex);

        sphereShader.use();

        // transformação
        model = glm::mat4(1.0f);
        // posição do planeta
        xx = sin(glfwGetTime() * plan_velocidade * 0.42f) * planet_raio * 7.f * 2.5f;
        zz = cos(glfwGetTime() * plan_velocidade * 0.42f) * planet_raio * 7.f * 2.5f;
        posicaoPlanetas[6] = glm::vec3(xx, 0, zz);

        model = glm::translate(model, glm::vec3(xx, -1.0f, zz));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));


        //###################################### NEPTUNO ######################################
        glBindVertexArray(vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, neptuno_tex);

        sphereShader.use();


        // transformação 
        model = glm::mat4(1.0f);
        // posição do planeta
        xx = sin(glfwGetTime() * plan_velocidade * 0.38f) * planet_raio * 8.f * 2.5f;
        zz = cos(glfwGetTime() * plan_velocidade * 0.38f) * planet_raio * 8.f * 2.5f;
        posicaoPlanetas[7] = glm::vec3(xx, 0, zz);

        model = glm::translate(model, glm::vec3(xx, 0.0f, zz));
        model = glm::rotate(model, (float)glfwGetTime() + 2, glm::vec3(0.0f, 3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
        sphereShader.setMat4("model", model);

        glDrawElements(GL_TRIANGLES, numIndicies, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

        // ###################################### orbitas ######################################
        glBindVertexArray(VAO_t);

        // mudar a cor da orbita
        glm::mat4 modelorb = glm::mat4(1.0f);

        for (int i = 1; i < 9; i++)
        {
            modelorb = glm::mat4(1);
            // iniciar no meio para fazer o circulo conforme o meio
            modelorb = glm::translate(modelorb, posicao_centro);
            if (i <= 4) {
                modelorb = glm::scale(modelorb, glm::vec3(i * 2.f, i * 2.f, i * 2.f));
            }
            else {
                modelorb = glm::scale(modelorb, glm::vec3(i * 2.5f, i * 2.5f, i * 2.5f));
            }
            sphereShader.setMat4("model", modelorb);
            glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbVert.size() / 3);

        }

        modelorb = glm::mat4(1.0f);
        modelorb = glm::translate(modelorb, posicaoPlanetas[2]);
        modelorb = glm::scale(modelorb, glm::vec3(0.5f * 2.f, 0, 0.5f * 2.f));
        sphereShader.setMat4("model", modelorb);
        glDrawArrays(GL_LINE_LOOP, 0, (GLsizei)orbVert.size() / 3);



        //###################################### SKYBOX ######################################
        glBindVertexArray(skyboxVAO);


        glDepthFunc(GL_LEQUAL);  // mudar a profundidade para que o skybox seja desenhado atrás de tudo

        skyboxShader.use();
        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // removemos a translacao da matriz view
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
        // ###################################### Info dos Planetas ######################################
        glm::vec3 pos_aux{};

        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS)
        {
            planeta = 0;
        }
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
        {
            Info.Nome = "Mercurio";
            Info.Velocidade = "47,87";
            Info.Massa = "0.32868";
            Info.Diametro = "4880";

            if (planeta != 1) {
                printPlanetInfo();
            }

            planeta = 1;
        }
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
        {
            Info.Nome = "Venus";
            Info.Velocidade = "35,02";
            Info.Massa = "0.32868";
            Info.Diametro = "12104";

            if (planeta != 2) {
                printPlanetInfo();
            }

            planeta = 2;
        }
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
        {
            Info.Nome = "Terra";
            Info.Velocidade = "29,78";
            Info.Massa = "5.97600";
            Info.Diametro = "12756";

            if (planeta != 3) {
                printPlanetInfo();
            }

            planeta = 3;
        }
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
        {
            Info.Nome = "Marte";
            Info.Velocidade = "24,13";
            Info.Massa = "0.63345";
            Info.Diametro = "6794";

            if (planeta != 4) {
                printPlanetInfo();
            }

            planeta = 4;
        }
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
        {
            Info.Nome = "Jupiter";
            Info.Velocidade = "13,07";
            Info.Massa = "1876.64328";
            Info.Diametro = "142984";

            if (planeta != 5) {
                printPlanetInfo();
            }

            planeta = 5;
        }
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
        {
            Info.Nome = "Saturno";
            Info.Velocidade = "9,69";
            Info.Massa = "561.80376";
            Info.Diametro = "120536";

            if (planeta != 6) {
                printPlanetInfo();
            }

            planeta = 6;
        }
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
        {
            Info.Nome = "Urano";
            Info.Velocidade = "6,81";
            Info.Massa = "86.05440";
            Info.Diametro = "51118";

            if (planeta != 7) {
                printPlanetInfo();
            }

            planeta = 7;
        }
        if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
        {
            Info.Nome = "Neptuno";
            Info.Velocidade = "5,43";
            Info.Massa = "101.59200";
            Info.Diametro = "49 532";

            if (planeta != 8) {
                printPlanetInfo();
            }

            planeta = 8;
        }
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        {
            planeta = 0;
        }


        switch (planeta)
        {
        case 1: // Mercurio
            pos_aux = posicaoPlanetas[0];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);

            break;
        case 2: // Venus
            pos_aux = posicaoPlanetas[1];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);

            break;
        case 3:
            pos_aux = posicaoPlanetas[2];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);

            break;
        case 4: // marte
            pos_aux = posicaoPlanetas[3];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);

            break;
        case 5: // jupiter
            pos_aux = posicaoPlanetas[4];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);

            break;
        case 6: // saturno
            pos_aux = posicaoPlanetas[5];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);

            break;
        case 7: // urano
            pos_aux = posicaoPlanetas[6];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);

            break;
        case 8: // netuno
            pos_aux = posicaoPlanetas[7];
            pos_aux.y = 40.0f;
            camera.Position = pos_aux;
            // camera olhar para baixo 
            camera.Pitch = -90.0f;
            camera.Yaw = 0.0f;
            camera.ProcessMouseMovement(0.0, 0.0);
        default:
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminar e limpar
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO_t);
    glDeleteBuffers(1, &VBO_t);
    glfwTerminate();

    return 0;
}

// processar todos os inputs do usuario
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    // zoom in com o z e zoom out com o x
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
        float Zoom = camera.Zoom;
        if (Zoom > 45.0f)
            camera.Zoom = 45.0f;
        else {
            camera.Zoom += 0.02f;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
    {
        float Zoom = camera.Zoom;
        if (Zoom < 1.0f)
            camera.Zoom = 1.0f;
        else {
            camera.Zoom -= 0.02f;
        }
    }
}

// glfw: quando a janela e alterada de tamanho (por exemplo, minimizada ou maximizada)
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: quando o mouse se move
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: quando o scroll do mouse e usado
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}
// carregar as texturas
// ----------------------
unsigned int loadTexture(char const* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 4)
            format = GL_RGBA;
        else if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
    }
    else
    {
        cout << "Texture failed to load at path: " << path << endl;
        stbi_image_free(data);
    }

    return textureID;
}
// Carregar os atributos necessarios para a criação de uma esfera
// ------------ 
GLuint SolidSphere(float radius, int slices, int stacks)
{
    using namespace glm;

    const float pi = 3.1415926535897932384626433832795f;
    const float _2pi = 2.0f * pi;

    //arrays que servem para guardar as coordenadas dos vertices, normais e coordenadas da textura
    vector<vec3> positions;
    vector<vec3> normals;
    vector<vec2> textureCoords;

    //Aqui calculamos as coordenadas UV da textura, como a estrutura a ser desenhada ira ser uma esfera precisamos de converter estas coordenadas para coordenadas esfericas, porque caso contrario estas coordenadas nao coincidem com a estrutura da textura a ser mapeada
    for (int i = 0; i <= stacks; ++i)
    {
        // V texture coordinate.
        float V = i / (float)stacks;
        float phi = V * pi;

        for (int j = 0; j <= slices; ++j)
        {
            // U texture coordinate.
            float U = j / (float)slices;
            float theta = U * _2pi;

            float X = cos(theta) * sin(phi);
            float Y = cos(phi);
            float Z = sin(theta) * sin(phi);

            positions.push_back(vec3(X, Y, Z) * radius);
            normals.push_back(vec3(X, Y, Z));
            textureCoords.push_back(vec2(U, V));
        }
    }

    // Aqui geramos os indices de posicao
    vector<GLuint> indicies;

    for (int i = 0; i < slices * stacks + slices; ++i)
    {
        indicies.push_back(i);
        indicies.push_back(i + slices + 1);
        indicies.push_back(i + slices);

        indicies.push_back(i + slices + 1);
        indicies.push_back(i);
        indicies.push_back(i + 1);
    }

    //Geramos um VAO para guardarmos os VBO a serem criados
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    //Criamos aqui 4 VBO para guardarmos as coordenadas dos vertices , normais , texturas e indices de posicoes
    GLuint vbos[4];
    glGenBuffers(4, vbos);

    //VBO das posicoes
    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(vec3), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(0);

    //VBO das normais
    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(vec3), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(1);

    //VBO das texturas
    glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
    glBufferData(GL_ARRAY_BUFFER, textureCoords.size() * sizeof(vec2), textureCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(2);

    //VBO dos indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[3]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicies.size() * sizeof(GLuint), indicies.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return vao;
}
// carregar textura de um cubo
//funcao que carrega multiplas strings com o caminho das texturas para cada uma das faces do cubo
unsigned int loadCubemap(vector<string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    //variaveis que recebem a altura e a largura tambem como o formato dos pixeis
    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        //utilizamos aqui a funcao da biblioteca stb_image
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            cout << "Cubemap texture failed to load at path: " << faces[i] << endl;
            stbi_image_free(data);
        }
    }

    //parametros da textura
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
// função para renderizar texto
void renderText(Shader& s, std::string text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state	
    s.use();
    glUniform3f(glGetUniformLocation(s.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);


    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;
        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };
        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void printPlanetInfo() {
    std::cout << "Planeta: " + Info.Nome << endl;
    std::cout << "Velocidade (km/s): " + Info.Velocidade << endl;
    std::cout << "Massa (kg * 10^24): " + Info.Massa << endl;
    std::cout << "Diametro (km): " + Info.Diametro << endl;
    std::cout << endl;
}
