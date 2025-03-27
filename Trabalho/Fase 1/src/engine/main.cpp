#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include "tinyxml2.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

using namespace std;
using namespace tinyxml2;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 (M_PI / 2.0)
#endif

// -----------------------------------------------------------------------------
// CÂMERA (Foi integrado da Aula2.cpp jovens)
// -----------------------------------------------------------------------------
struct Camera {
    float eye[3];    // Posição da câmara
    float center[3]; // Ponto de interesse (fixo em 0,0,0)
    float up[3];     // Vetor "up"
    float yaw;       // Rotação horizontal (em torno do eixo Y)
    float pitch;     // Rotação vertical (em torno do eixo X)
    float distance;  // Distância do ponto (0,0,0)

    Camera() {
        yaw = 0.0f;
        pitch = 0.0f;
        distance = 5.0f;
        center[0] = center[1] = center[2] = 0.0f;
        up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f;
        updatePosition();
    }

    // Atualiza a posição (eye) com base em yaw, pitch e distance
    void updatePosition() {
        eye[0] = distance * cos(pitch) * cos(yaw);
        eye[1] = distance * sin(pitch);
        eye[2] = distance * cos(pitch) * sin(yaw);
        // Atualiza o vetor up (pode ser ajustado se necessário)
        up[0] = -sin(pitch) * cos(yaw);
        up[1] = cos(pitch);
        up[2] = -sin(pitch) * sin(yaw);
    }

    void rotateYaw(float angle) {
        yaw += angle;
        updatePosition();
    }

    void rotatePitch(float angle) {
        pitch += angle;
        if (pitch > M_PI_2 - 0.1f) pitch = M_PI_2 - 0.1f;
        if (pitch < -M_PI_2 + 0.1f) pitch = -M_PI_2 + 0.1f;
        updatePosition();
    }

    void zoom(float amount) {
        distance += amount;
        if (distance < 2.0f) distance = 2.0f;
        if (distance > 15.0f) distance = 15.0f;
        updatePosition();
    }
};

Camera camera; // Instância global da câmara

// -----------------------------------------------------------------------------
// ESTRUTURA PARA ARMAZENAR OS DADOS DO XML
// -----------------------------------------------------------------------------
struct World {
    // Camera data read from XML
    float cameraPosition[3];
    float cameraLookAt[3];
    float cameraUp[3];
    float cameraFov;
    float cameraNear;
    float cameraFar;
    // List of model filenames
    vector<string> files;
    // Each model's vertices stored as a vector of floats (x, y, z)
    vector< vector<float> > vertices;
    // Removed color assignment for primitives.
};

World world;

// -----------------------------------------------------------------------------
// FUNÇÕES DE DESENHO E CALLBACKS (integrados da Aula2)
// -----------------------------------------------------------------------------

// Desenha os eixos (X em vermelho, Y em verde, Z em azul)
void drawAxis() {
    glPushAttrib(GL_CURRENT_BIT);

    glBegin(GL_LINES);
    // Eixo X
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-100.0f, 0.0f, 0.0f);
    glVertex3f(100.0f, 0.0f, 0.0f);
    // Eixo Y
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -100.0f, 0.0f);
    glVertex3f(0.0f, 100.0f, 0.0f);
    // Eixo Z
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, -100.0f);
    glVertex3f(0.0f, 0.0f, 100.0f);
    glEnd();

    glPopAttrib();
}

// Função que redimensiona da janela (mantém a perspectiva)
void changeSize(int w, int h) {
    if (h == 0)
        h = 1;
    float ratio = w * 1.0f / h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

// Função auxiliar para converter letras maiúsculas em minúsculas
// teve que ser, o tolower() não estava a dar
char lowerMyKey(unsigned char key) {
    return (key >= 'A' && key <= 'Z') ? key + 0x20 : key;
}

// Callback do teclado para mover a câmera
// a/d: rotação horizontal; w/s: rotação vertical; x/z: zoom; ESC: sair.
void keyboard(unsigned char key, int x, int y) {
    float rotateStep = 0.05f;
    float zoomStep = 0.3f;
    switch (lowerMyKey(key)) {
    case 'a': camera.rotateYaw(-rotateStep); break;
    case 'd': camera.rotateYaw(rotateStep); break;
    case 'w': camera.rotatePitch(rotateStep); break;
    case 's': camera.rotatePitch(-rotateStep); break;
    case 'x': camera.zoom(-zoomStep); break;
    case 'z': camera.zoom(zoomStep); break;
    case 27: exit(0); break; // ESC
    default: break;
    }
    glutPostRedisplay();
}

// Callback que processa as teclas especiais
void specialKeys(int key, int x, int y) {
    glutPostRedisplay();
}

// -----------------------------------------------------------------------------
// FUNÇÕES PARA LEITURA DOS MODELOS
// -----------------------------------------------------------------------------

// Lê um ficheiro de modelo (formato: número de vértices seguido de linhas com "x y z")
// Os ficheiros estao na pasta "models/generated/"
bool readFile(const string& filename) {
    // From engine\build\Release, go up three levels to Trabalho\src, then into generator\models\generated
    
    string fullFilePath = "../../models/generated/" + filename;

    ifstream infile(fullFilePath);
    if (!infile) return false;
    int num;
    infile >> num;
    vector<float> modelVertices;
    for (int i = 0; i < num; i++) {
        float x, y, z;
        infile >> x >> y >> z;
        modelVertices.push_back(x);
        modelVertices.push_back(y);
        modelVertices.push_back(z);
    }
    world.vertices.push_back(modelVertices);
    return true;
}

// -----------------------------------------------------------------------------
// FUNÇÃO PARA FAZER PARSE DO XML DE CONFIGURAÇÃO
// -----------------------------------------------------------------------------
bool parseXML(const char* xmlFile) {
    XMLDocument xmlDoc;
    XMLError eResult = xmlDoc.LoadFile(xmlFile);
    if (eResult != XML_SUCCESS) {
        cerr << "Erro ao carregar o XML: " << xmlDoc.ErrorStr() << endl;
        return false;
    }
    XMLElement* pRoot = xmlDoc.FirstChildElement("world");
    if (!pRoot) {
        cerr << "Formato XML inválido: elemento 'world' não encontrado." << endl;
        return false;
    }
    // Lê os dados da janela (opcional, mas obrigatório no xml_syntax.xml)
    XMLElement* pWindow = pRoot->FirstChildElement("window");
    if (!pWindow) {
        cerr << "Elemento 'window' não encontrado." << endl;
        return false;
    }
    // Lê os dados da câmara
    XMLElement* pCamera = pRoot->FirstChildElement("camera");
    if (pCamera) {
        XMLElement* pPos = pCamera->FirstChildElement("position");
        if (pPos) {
            pPos->QueryFloatAttribute("x", &world.cameraPosition[0]);
            pPos->QueryFloatAttribute("y", &world.cameraPosition[1]);
            pPos->QueryFloatAttribute("z", &world.cameraPosition[2]);
        }
        XMLElement* pLookAt = pCamera->FirstChildElement("lookAt");
        if (pLookAt) {
            pLookAt->QueryFloatAttribute("x", &world.cameraLookAt[0]);
            pLookAt->QueryFloatAttribute("y", &world.cameraLookAt[1]);
            pLookAt->QueryFloatAttribute("z", &world.cameraLookAt[2]);
        }
        XMLElement* pUp = pCamera->FirstChildElement("up");
        if (pUp) {
            pUp->QueryFloatAttribute("x", &world.cameraUp[0]);
            pUp->QueryFloatAttribute("y", &world.cameraUp[1]);
            pUp->QueryFloatAttribute("z", &world.cameraUp[2]);
        }
        XMLElement* pProj = pCamera->FirstChildElement("projection");
        if (pProj) {
            pProj->QueryFloatAttribute("fov", &world.cameraFov);
            pProj->QueryFloatAttribute("near", &world.cameraNear);
            pProj->QueryFloatAttribute("far", &world.cameraFar);
        }
    }
    // Lê os modelos (dentro do grupo)
    XMLElement* pGroup = pRoot->FirstChildElement("group");
    if (pGroup) {
        // Ignora o elemento "transform" se existir e procura pelo elemento "models"
        XMLElement* pModels = pGroup->FirstChildElement("models");
        if (pModels) {
            XMLElement* pModel = pModels->FirstChildElement("model");
            while (pModel) {
                const char* fileAttr = pModel->Attribute("file");
                if (fileAttr) {
                    string fname = fileAttr;
                    world.files.push_back(fname);
                    if (!readFile(fname)) {
                        cerr << "Erro ao ler o ficheiro do modelo: " << fname << endl;
                        return false;
                    }
                }
                pModel = pModel->NextSiblingElement("model");
            }
        }
    }
    return true;
}

// -----------------------------------------------------------------------------
// FUNÇÃO QUE DESENHA OS MODELOS CARREGADOS (em wireframe)
// -----------------------------------------------------------------------------
void drawPrimitives() {
    glPushAttrib(GL_CURRENT_BIT);
    // Draw each model without applying a specific color.
    for (size_t i = 0; i < world.vertices.size(); ++i) {
        glBegin(GL_TRIANGLES);
        // No glColor call here; the default color is used.
        const auto& modelVertices = world.vertices[i];
        for (size_t j = 0; j < modelVertices.size(); j += 3) {
            glVertex3f(modelVertices[j], modelVertices[j + 1], modelVertices[j + 2]);
        }
        glEnd();
    }
    glPopAttrib();
}

// -----------------------------------------------------------------------------
// FUNÇÃO DE RENDERIZAÇÃO
// -----------------------------------------------------------------------------
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    // Define a vista usando a câmara integrada
    gluLookAt(camera.eye[0], camera.eye[1], camera.eye[2],
        camera.center[0], camera.center[1], camera.center[2],
        camera.up[0], camera.up[1], camera.up[2]);
    // Set polygon mode to draw only the lines (wireframe)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawAxis();
    drawPrimitives();
    glutSwapBuffers();
}

// -----------------------------------------------------------------------------
// FUNÇÃO MAIN
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    // Se nenhum argumento for passado, usa o ficheiro "inputs/xml_syntax.xml" por padrão.
    const char* xmlFile = "../../engine/inputs/test_1_5.xml";
    

    if (!parseXML(xmlFile)) {
        cout << "Erro ao ler o ficheiro de configuração XML." << endl;
        return 1;
    }

    // Atualiza a câmara global com os valores lidos do XML
    camera.eye[0] = world.cameraPosition[0];
    camera.eye[1] = world.cameraPosition[1];
    camera.eye[2] = world.cameraPosition[2];

    camera.center[0] = world.cameraLookAt[0];
    camera.center[1] = world.cameraLookAt[1];
    camera.center[2] = world.cameraLookAt[2];

    camera.up[0] = world.cameraUp[0];
    camera.up[1] = world.cameraUp[1];
    camera.up[2] = world.cameraUp[2];

    // Calcula a distância e os ângulos iniciais para a câmara
    camera.distance = sqrt(camera.eye[0] * camera.eye[0] +
        camera.eye[1] * camera.eye[1] +
        camera.eye[2] * camera.eye[2]);
    camera.pitch = asin(camera.eye[1] / camera.distance);
    camera.yaw = atan2(camera.eye[2], camera.eye[0]);
    camera.updatePosition();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Engine");
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glutMainLoop();
    return 0;
}
