#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <GL/glut.h>
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

// -----------------------------------------------------------------------------
// CÂMERA (Foi integrado da Aula2.cpp jovens)
// -----------------------------------------------------------------------------
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 (M_PI / 2.0)
#endif

struct Camera {
    float eye[3];    // Posição da câmara
    float center[3]; // Ponto de interesse
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
        // Atualiza o vetor up
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
        if (pitch > M_PI_2 - 0.1f)  pitch = M_PI_2 - 0.1f;
        if (pitch < -M_PI_2 + 0.1f) pitch = -M_PI_2 + 0.1f;
        updatePosition();
    }
    void zoom(float amount) {
        distance += amount;
        updatePosition();
    }

};

Camera camera; // Instância global da câmara

// -----------------------------------------------------------------------------
// ESTRUTURAS PARA O GRAFO DE CENA
// -----------------------------------------------------------------------------

// Vetor 3D simples
struct Vec3 {
    float x, y, z;
};

// Tipo de transformação (para preservar a ordem exata de <translate>, <rotate>, <scale>)
enum class TransformType { TRANSLATE, ROTATE, SCALE };

// Representa uma transformação individual
struct SingleTransform {
    TransformType type;
    float angle;  // usado apenas se type == ROTATE
    Vec3 xyz;     // x,y,z usados para translate/rotate/scale
};

// Cada nó de cena terá uma lista ordenada de transformações, seus modelos e filhos
struct ModelData {
    string fileName;
    Vec3 localTranslation = { 0.0f, 0.0f, 0.0f };
};


struct SceneNode {
    vector<SingleTransform> transforms; // lista ordenada de transformações
    vector<ModelData> models;
    vector<SceneNode> children;
};

// Estrutura principal da cena, contendo todos os nós de topo e a “biblioteca” de vértices
struct Scene {
    vector<SceneNode> rootNodes;
    map<string, vector<Vec3>> modelLibrary; // (ficheiro -> lista de vértices)
};

Scene scene;

// -----------------------------------------------------------------------------
// FUNÇÃO PARA CARREGAR OS MODELOS A PARTIR DE FICHEIROS
// -----------------------------------------------------------------------------
bool loadModelFile(const string& fname) {
    // Supondo que os modelos estão em "models/generated/"
    string fullPath = "../../models/generated/" + fname;
    ifstream inFile(fullPath);
    if (!inFile) return false;

    vector<Vec3> vertices;
    int numVerts;
    inFile >> numVerts;
    for (int i = 0; i < numVerts; i++) {
        Vec3 pt;
        inFile >> pt.x >> pt.y >> pt.z;
        vertices.push_back(pt);
    }
    scene.modelLibrary[fname] = vertices;
    return true;
}

// -----------------------------------------------------------------------------
// FUNÇÃO PARA DESENHAR OS MODELOS (SEM cores aleatórias, apenas linhas brancas)
// -----------------------------------------------------------------------------
void renderModel(const ModelData& mdl) {
    auto it = scene.modelLibrary.find(mdl.fileName);
    if (it == scene.modelLibrary.end()) return;

    const vector<Vec3>& verts = it->second;
    glBegin(GL_TRIANGLES);
    glColor3f(1.0f, 1.0f, 1.0f); // linhas brancas
    for (auto& v : verts) {
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();
}

// -----------------------------------------------------------------------------
// DESENHAR EIXOS (apenas para referência, coloridos)
// -----------------------------------------------------------------------------
void drawAxes() {
    glBegin(GL_LINES);
    // Eixo X em vermelho
    glColor3f(1, 0, 0);
    glVertex3f(-15, 0, 0); glVertex3f(15, 0, 0);
    // Eixo Y em verde
    glColor3f(0, 1, 0);
    glVertex3f(0, -15, 0); glVertex3f(0, 15, 0);
    // Eixo Z em azul
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, -15); glVertex3f(0, 0, 15);
    glEnd();
}

// -----------------------------------------------------------------------------
// APLICA TODAS AS TRANSFORMAÇÕES (NA ORDEM DEFINIDA NO XML)
// -----------------------------------------------------------------------------
void applyTransformations(const vector<SingleTransform>& transforms) {
    // Aplica cada transformação na ordem em que aparecem no XML
    for (auto& t : transforms) {
        switch (t.type) {
        case TransformType::TRANSLATE:
            glTranslatef(t.xyz.x, t.xyz.y, t.xyz.z);
            break;
        case TransformType::ROTATE:
            glRotatef(t.angle, t.xyz.x, t.xyz.y, t.xyz.z);
            break;
        case TransformType::SCALE:
            glScalef(t.xyz.x, t.xyz.y, t.xyz.z);
            break;
        }
    }
}

// -----------------------------------------------------------------------------
// DESENHAR UM NÓ DA CENA (RECURSIVO)
// -----------------------------------------------------------------------------
void drawSceneNode(const SceneNode& node) {
    glPushMatrix();
    // Aplica todas as transformações do nó, na ordem do XML
    applyTransformations(node.transforms);

    // Desenha cada modelo do nó
    for (auto& mdl : node.models) {
        glPushMatrix();
        // Se o <model> tiver um <translate> interno
        if (mdl.localTranslation.x != 0.0f ||
            mdl.localTranslation.y != 0.0f ||
            mdl.localTranslation.z != 0.0f) {
            glTranslatef(mdl.localTranslation.x, mdl.localTranslation.y, mdl.localTranslation.z);
        }
        renderModel(mdl);
        glPopMatrix();
    }

    // Desenha recursivamente os filhos
    for (auto& child : node.children) {
        drawSceneNode(child);
    }
    glPopMatrix();
}

// -----------------------------------------------------------------------------
// FUNÇÃO DE RENDERIZAÇÃO
// -----------------------------------------------------------------------------
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camera.eye[0], camera.eye[1], camera.eye[2],
        camera.center[0], camera.center[1], camera.center[2],
        camera.up[0], camera.up[1], camera.up[2]);

    // Modo wireframe: desenha apenas linhas
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Desenha eixos (opcional)
    drawAxes();

    // Desenha cada nó de cena raiz
    for (auto& node : scene.rootNodes) {
        drawSceneNode(node);
    }
    glutSwapBuffers();
}

// -----------------------------------------------------------------------------
// FUNÇÃO QUE REDIMENSIONA A JANELA (mantém a perspectiva)
// -----------------------------------------------------------------------------
void changeSize(int w, int h) {
    if (h == 0) h = 1;
    float ratio = (float)w / (float)h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

// -----------------------------------------------------------------------------
// FUNÇÃO DE LEITURA DE VETORES (x,y,z) PARA TRANSFORMAÇÕES
// -----------------------------------------------------------------------------
Vec3 parseXYZ(XMLElement* elem) {
    Vec3 v{ 0,0,0 };
    if (!elem) return v;
    elem->QueryFloatAttribute("x", &v.x);
    elem->QueryFloatAttribute("y", &v.y);
    elem->QueryFloatAttribute("z", &v.z);
    return v;
}

// -----------------------------------------------------------------------------
// FUNÇÃO RECURSIVA PARA LER <group> E SUAS SUB-TAGS (phase 2)
// -----------------------------------------------------------------------------
bool parseGroup(XMLElement* groupElem, SceneNode& node) {
    // Achar <transform> e ler cada sub-elemento na ORDEM
    if (XMLElement* transElem = groupElem->FirstChildElement("transform")) {
        for (XMLElement* t = transElem->FirstChildElement(); t; t = t->NextSiblingElement()) {
            string tname = t->Name();
            if (tname == "translate") {
                SingleTransform st;
                st.type = TransformType::TRANSLATE;
                st.xyz = parseXYZ(t);
                node.transforms.push_back(st);
            }
            else if (tname == "rotate") {
                SingleTransform st;
                st.type = TransformType::ROTATE;
                t->QueryFloatAttribute("angle", &st.angle);
                st.xyz = parseXYZ(t);
                node.transforms.push_back(st);
            }
            else if (tname == "scale") {
                SingleTransform st;
                st.type = TransformType::SCALE;
                st.xyz = parseXYZ(t);
                node.transforms.push_back(st);
            }
        }
    }
    // Lê os modelos
    if (XMLElement* modelsElem = groupElem->FirstChildElement("models")) {
        for (XMLElement* mod = modelsElem->FirstChildElement("model"); mod; mod = mod->NextSiblingElement("model")) {
            ModelData mdl;
            const char* fname = mod->Attribute("file");
            if (fname) {
                mdl.fileName = fname;
                // Carregar se não existir
                if (scene.modelLibrary.find(mdl.fileName) == scene.modelLibrary.end()) {
                    if (!loadModelFile(mdl.fileName)) {
                        cerr << "Erro ao carregar modelo: " << mdl.fileName << endl;
                        return false;
                    }
                }
                // Se tiver <translate> interno
                if (XMLElement* localT = mod->FirstChildElement("translate")) {
                    mdl.localTranslation = parseXYZ(localT);
                }
                node.models.push_back(mdl);
            }
        }
    }
    // Lê sub-grupos recursivamente
    for (XMLElement* childGrp = groupElem->FirstChildElement("group"); childGrp; childGrp = childGrp->NextSiblingElement("group")) {
        SceneNode childNode;
        if (!parseGroup(childGrp, childNode)) return false;
        node.children.push_back(childNode);
    }
    return true;
}


// Função auxiliar para os parametros da camera
void updateCameraParameters(Camera& cam) {
    // Calcula a diferença entre a posição da câmera (eye) e o ponto de interesse (center)
    float dx = cam.eye[0] - cam.center[0];
    float dy = cam.eye[1] - cam.center[1];
    float dz = cam.eye[2] - cam.center[2];

    // Atualiza a distância (magnitude do vetor)
    cam.distance = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Calcula yaw (ângulo no plano XZ)
    cam.yaw = std::atan2(dz, dx);

    // Calcula pitch (ângulo de elevação)
    cam.pitch = std::asin(dy / cam.distance);

    // Atualiza a posição (eye) e o vetor up com base nos novos parâmetros
    cam.updatePosition();
}


// -----------------------------------------------------------------------------
// FUNÇÃO PARA FAZER PARSE DO XML (SOMENTE PHASE 2)
// -----------------------------------------------------------------------------
bool parseXML(const char* xmlFile) {
    using namespace tinyxml2;
    XMLDocument doc;
    if (doc.LoadFile(xmlFile) != XML_SUCCESS) {
        cerr << "Falha ao abrir XML: " << xmlFile << endl;
        return false;
    }
    XMLElement* root = doc.FirstChildElement("world");
    if (!root) {
        cerr << "Elemento <world> não encontrado." << endl;
        return false;
    }
    // Lê a câmara
    if (XMLElement* cam = root->FirstChildElement("camera")) {
        if (XMLElement* pos = cam->FirstChildElement("position")) {
            pos->QueryFloatAttribute("x", &camera.eye[0]);
            pos->QueryFloatAttribute("y", &camera.eye[1]);
            pos->QueryFloatAttribute("z", &camera.eye[2]);
        }
        if (XMLElement* la = cam->FirstChildElement("lookAt")) {
            la->QueryFloatAttribute("x", &camera.center[0]);
            la->QueryFloatAttribute("y", &camera.center[1]);
            la->QueryFloatAttribute("z", &camera.center[2]);
        }
        if (XMLElement* up = cam->FirstChildElement("up")) {
            up->QueryFloatAttribute("x", &camera.up[0]);
            up->QueryFloatAttribute("y", &camera.up[1]);
            up->QueryFloatAttribute("z", &camera.up[2]);
        }
      
        updateCameraParameters(camera);
    }

    // Lê cada <group> do <world>
    for (XMLElement* grp = root->FirstChildElement("group"); grp; grp = grp->NextSiblingElement("group")) {
        SceneNode rootNode;
        if (!parseGroup(grp, rootNode)) return false;
        scene.rootNodes.push_back(rootNode);
    }
    return true;
}

// -----------------------------------------------------------------------------
// CALLBACK PARA TECLAS (CONTROLO DE CÂMERA)
// -----------------------------------------------------------------------------
void processKeys(unsigned char key, int x, int y) {
    float rotateStep = 0.05f;
    float zoomStep = 0.3f;
    switch (key) {
    case 'a': camera.rotateYaw(-rotateStep);  break;
    case 'd': camera.rotateYaw(rotateStep);   break;
    case 'w': camera.rotatePitch(rotateStep); break;
    case 's': camera.rotatePitch(-rotateStep);break;
    case 'x': camera.zoom(-zoomStep);         break;
    case 'z': camera.zoom(zoomStep);          break;
    case 27:  exit(0);                        break; // ESC
    default:  break;
    }
    glutPostRedisplay();
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------
int main(int argc, char** argv) {
    // Se nenhum argumento for passado, usa o ficheiro "inputs/xml_syntax.xml" por padr�o.
    const char* xmlFile = "../../engine/inputs/test_2_3.xml";


    if (!parseXML(xmlFile)) {
        cout << "Erro ao ler o ficheiro de configura��o XML." << endl;
        return 1;
    }

    // Inicialização do GLUT / OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Engine 3D - Phase 2");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Callbacks
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(processKeys);

    // Loop principal
    glutMainLoop();
    return 0;
}
