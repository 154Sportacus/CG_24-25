#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
#include <GL/gl.h>
#include <GL/glew.h> 
#include <GL/glut.h>
#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 (M_PI / 2.0)
#endif

// ---------------------------------------------------------------------------
// Variáveis globais para o tamanho da janela
// ---------------------------------------------------------------------------
int gWindowWidth = 800;
int gWindowHeight = 600;

// ---------------------------------------------------------------------------
// Câmara
// ---------------------------------------------------------------------------
struct Camera {
    float eye[3];    // posição da câmara
    float center[3]; // ponto para onde olha
    float up[3];     // vetor up
    float yaw;       // rotação horizontal (em torno de Y)
    float pitch;     // rotação vertical (em torno de X)
    float distance;  // distância ao centro

    Camera() {
        yaw = pitch = 0.0f;
        distance = 5.0f;
        center[0] = center[1] = center[2] = 0.0f;
        up[0] = 0.0f; up[1] = 1.0f; up[2] = 0.0f;
        updatePosition();
    }
    void updatePosition() {
        eye[0] = distance * cos(pitch) * cos(yaw);
        eye[1] = distance * sin(pitch);
        eye[2] = distance * cos(pitch) * sin(yaw);
        up[0] = -sin(pitch) * cos(yaw);
        up[1] = cos(pitch);
        up[2] = -sin(pitch) * sin(yaw);
    }
    void rotateYaw(float a) { yaw += a; updatePosition(); }
    void rotatePitch(float a) { pitch += a; if (pitch > M_PI_2 - 0.1f) pitch = M_PI_2 - 0.1f; if (pitch < -M_PI_2 + 0.1f) pitch = -M_PI_2 + 0.1f; updatePosition(); }
    void zoom(float d) { distance += d; updatePosition(); }
};
Camera camera;

// ---------------------------------------------------------------------------
// Estruturas do grafo de cena
// ---------------------------------------------------------------------------
struct Vec3 { float x, y, z; };
enum class TransformType { TRANSLATE, ROTATE, SCALE, TRANSLATE_PATH, ROTATE_TIME };
struct SingleTransform {
    TransformType type;
    Vec3 xyz = { 0,0,0 };      // valor ou eixo
    float angle = 0.0f;        // para ROTATE
    float time = 0.0f;         // segundos de ciclo
    bool align = false;        // alinhar à tangente?
    vector<Vec3> path;         // pontos de controlo (Catmull–Rom)
};

struct ModelData {
    string fileName;
    Vec3 localTranslation = { 0,0,0 };
    GLuint vbo = 0;           // VBO handle
    int vertexCount = 0;      // número de vértices
};
struct SceneNode {
    vector<SingleTransform> transforms;
    vector<ModelData> models;
    vector<SceneNode> children;
};
struct Scene {
    vector<SceneNode> rootNodes;
    map<string, vector<Vec3>> modelLibrary;
} scene;

// Mapeamento de VBOs por nome de ficheiro
static map<string, pair<GLuint, int>> modelVBOs;

// -----------------------------------------------------------------------------
// Funções de apoio a Catmull–Rom e vetores
// -----------------------------------------------------------------------------
static Vec3 cross(const Vec3& a, const Vec3& b) {
    return { a.y * b.z - a.z * b.y,
             a.z * b.x - a.x * b.z,
             a.x * b.y - a.y * b.x };
}
static Vec3 normalize(const Vec3& v) {
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return len > 1e-6f ? Vec3{ v.x / len, v.y / len, v.z / len } : Vec3{ 0,0,0 };
}
static void evalCatmullRom(const Vec3& P0, const Vec3& P1, const Vec3& P2, const Vec3& P3,
    float t, Vec3& pos, Vec3& der) {
    float t2 = t * t, t3 = t2 * t;
    float c0 = -0.5f * t3 + t2 - 0.5f * t;
    float c1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
    float c2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
    float c3 = 0.5f * t3 - 0.5f * t2;
    pos.x = P0.x * c0 + P1.x * c1 + P2.x * c2 + P3.x * c3;
    pos.y = P0.y * c0 + P1.y * c1 + P2.y * c2 + P3.y * c3;
    pos.z = P0.z * c0 + P1.z * c1 + P2.z * c2 + P3.z * c3;
    float d0 = -1.5f * t2 + 2.0f * t - 0.5f;
    float d1 = 4.5f * t2 - 5.0f * t;
    float d2 = -4.5f * t2 + 4.0f * t + 0.5f;
    float d3 = 1.5f * t2 - t;
    der.x = P0.x * d0 + P1.x * d1 + P2.x * d2 + P3.x * d3;
    der.y = P0.y * d0 + P1.y * d1 + P2.y * d2 + P3.y * d3;
    der.z = P0.z * d0 + P1.z * d1 + P2.z * d2 + P3.z * d3;
}

// -----------------------------------------------------------------------------
// Função de carregar modelo .3d e criar VBO
// -----------------------------------------------------------------------------
bool loadModelFile(const string& fname) {
    string path = "../../models/generated/" + fname;
    ifstream in(path);
    if (!in) return false;
    int n; in >> n;
    vector<Vec3> verts(n);
    for (int i = 0; i < n; ++i)
        in >> verts[i].x >> verts[i].y >> verts[i].z;

    // Armazenar em modelLibrary (para compatibilidade)
    scene.modelLibrary[fname] = verts;

    // Criar VBO
    GLuint buf = 0;
    glGenBuffers(1, &buf);
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(Vec3),
        verts.data(),
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Guardar handle e contagem
    modelVBOs[fname] = { buf, int(verts.size()) };
    return true;
}

// -----------------------------------------------------------------------------
// Renderização de um modelo com VBO
// -----------------------------------------------------------------------------
void renderModel(const ModelData& M) {
    auto it = modelVBOs.find(M.fileName);
    if (it == modelVBOs.end()) return;
    GLuint buf = it->second.first;
    int    count = it->second.second;

    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, count);
    glDisableClientState(GL_VERTEX_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// -----------------------------------------------------------------------------
// Desenha eixos
// -----------------------------------------------------------------------------
void drawAxes() {
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    glVertex3f(-15, 0, 0); glVertex3f(15, 0, 0);
    glColor3f(0, 1, 0);
    glVertex3f(0, -15, 0); glVertex3f(0, 15, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, -15); glVertex3f(0, 0, 15);
    glEnd();
}

// -----------------------------------------------------------------------------
// Aplica transformações (inclui path Catmull–Rom e rotação no tempo)
// -----------------------------------------------------------------------------
void applyTransformations(const vector<SingleTransform>& T) {
    for (auto& t : T) {
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
        case TransformType::TRANSLATE_PATH: {
            float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            float u = fmodf(now, t.time) / t.time;
            int seg = int(u * (t.path.size() - 3));
            int maxSeg = int(t.path.size()) - 4;
            if (seg < 0) seg = 0;
            if (seg > maxSeg) seg = maxSeg;
            float s = u * (t.path.size() - 3) - seg;
            Vec3 pos, der;
            evalCatmullRom(t.path[seg], t.path[seg + 1], t.path[seg + 2], t.path[seg + 3], s, pos, der);
            glTranslatef(pos.x, pos.y, pos.z);
            if (t.align) {
                Vec3 tan = normalize(der);
                Vec3 upv = { 0,1,0 };
                Vec3 sid = normalize(cross(tan, upv));
                upv = cross(sid, tan);
                GLfloat M[16] = { sid.x,sid.y,sid.z,0,
                                  upv.x,upv.y,upv.z,0,
                                  tan.x,tan.y,tan.z,0,
                                  0,0,0,1 };
                glMultMatrixf(M);
            }
        } break;
        case TransformType::ROTATE_TIME: {
            float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            float ang = fmodf(now, t.time) / t.time * 360.0f;
            glRotatef(ang, t.xyz.x, t.xyz.y, t.xyz.z);
        } break;
        }
    }
}

// -----------------------------------------------------------------------------
// Desenha um nó e recursivamente os filhos
// -----------------------------------------------------------------------------
void drawSceneNode(const SceneNode& N) {
    glPushMatrix();
    applyTransformations(N.transforms);
    for (auto& m : N.models) {
        glPushMatrix();
        if (m.localTranslation.x || m.localTranslation.y || m.localTranslation.z)
            glTranslatef(m.localTranslation.x, m.localTranslation.y, m.localTranslation.z);
        renderModel(m);
        glPopMatrix();
    }
    for (auto& c : N.children) drawSceneNode(c);
    glPopMatrix();
}

// -----------------------------------------------------------------------------
// Render Scene
// -----------------------------------------------------------------------------
void renderScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    gluLookAt(camera.eye[0], camera.eye[1], camera.eye[2],
        camera.center[0], camera.center[1], camera.center[2],
        camera.up[0], camera.up[1], camera.up[2]);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawAxes();
    for (auto& node : scene.rootNodes) drawSceneNode(node);
    glutSwapBuffers();
}

// -----------------------------------------------------------------------------
// Change Size
// -----------------------------------------------------------------------------
void changeSize(int w, int h) {
    if (h == 0) h = 1;
    float ratio = float(w) / float(h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

// -----------------------------------------------------------------------------
// Parse (x,y,z)
// -----------------------------------------------------------------------------
Vec3 parseXYZ(XMLElement* e) {
    Vec3 v = { 0,0,0 }; if (!e) return v;
    e->QueryFloatAttribute("x", &v.x);
    e->QueryFloatAttribute("y", &v.y);
    e->QueryFloatAttribute("z", &v.z);
    return v;
}

// -----------------------------------------------------------------------------
// Parser recursivo do <group>
// -----------------------------------------------------------------------------
bool parseGroup(XMLElement* g, SceneNode& node) {
    if (XMLElement* tr = g->FirstChildElement("transform")) {
        for (XMLElement* t = tr->FirstChildElement(); t; t = t->NextSiblingElement()) {
            string name = t->Name(); SingleTransform st;
            if (name == "translate") {
                if (t->Attribute("time")) {
                    st.type = TransformType::TRANSLATE_PATH;
                    st.time = t->FloatAttribute("time");
                    st.align = t->BoolAttribute("align");
                    for (XMLElement* pt = t->FirstChildElement("point"); pt; pt = pt->NextSiblingElement("point"))
                        st.path.push_back(parseXYZ(pt));
                }
                else {
                    st.type = TransformType::TRANSLATE;
                    st.xyz = parseXYZ(t);
                }
            }
            else if (name == "rotate") {
                if (t->Attribute("time")) {
                    st.type = TransformType::ROTATE_TIME;
                    st.time = t->FloatAttribute("time");
                    st.xyz = parseXYZ(t);
                }
                else {
                    st.type = TransformType::ROTATE;
                    st.angle = t->FloatAttribute("angle");
                    st.xyz = parseXYZ(t);
                }
            }
            else if (name == "scale") {
                st.type = TransformType::SCALE;
                st.xyz = parseXYZ(t);
            }
            node.transforms.push_back(st);
        }
    }
    if (XMLElement* ms = g->FirstChildElement("models")) {
        for (XMLElement* m = ms->FirstChildElement("model"); m; m = m->NextSiblingElement("model")) {
            ModelData md; const char* f = m->Attribute("file");
            if (f) {
                md.fileName = f;
                if (!scene.modelLibrary.count(md.fileName))
                    if (!loadModelFile(md.fileName)) return false;
                // recuperar VBO e contagem
                auto it = modelVBOs.find(md.fileName);
                if (it != modelVBOs.end()) {
                    md.vbo = it->second.first;
                    md.vertexCount = it->second.second;
                }
                if (XMLElement* lt = m->FirstChildElement("translate"))
                    md.localTranslation = parseXYZ(lt);
                node.models.push_back(md);
            }
        }
    }
    for (XMLElement* c = g->FirstChildElement("group"); c; c = c->NextSiblingElement("group")) {
        SceneNode ch; if (!parseGroup(c, ch)) return false; node.children.push_back(ch);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Parser XML completo
// -----------------------------------------------------------------------------
bool parseXML(const char* file) {
    XMLDocument doc;
    if (doc.LoadFile(file) != XML_SUCCESS) return false;
    XMLElement* w = doc.FirstChildElement("world");
    if (!w) return false;
    if (XMLElement* win = w->FirstChildElement("window")) {
        win->QueryIntAttribute("width", &gWindowWidth);
        win->QueryIntAttribute("height", &gWindowHeight);
    }
    if (XMLElement* cam = w->FirstChildElement("camera")) {
        if (XMLElement* p = cam->FirstChildElement("position")) {
            p->QueryFloatAttribute("x", &camera.eye[0]);
            p->QueryFloatAttribute("y", &camera.eye[1]);
            p->QueryFloatAttribute("z", &camera.eye[2]);
        }
        if (XMLElement* l = cam->FirstChildElement("lookAt")) {
            l->QueryFloatAttribute("x", &camera.center[0]);
            l->QueryFloatAttribute("y", &camera.center[1]);
            l->QueryFloatAttribute("z", &camera.center[2]);
        }
        if (XMLElement* u = cam->FirstChildElement("up")) {
            u->QueryFloatAttribute("x", &camera.up[0]);
            u->QueryFloatAttribute("y", &camera.up[1]);
            u->QueryFloatAttribute("z", &camera.up[2]);
        }
        camera.updatePosition();
    }
    for (XMLElement* g = w->FirstChildElement("group"); g; g = g->NextSiblingElement("group")) {
        SceneNode rn;
        if (!parseGroup(g, rn)) return false;
        scene.rootNodes.push_back(rn);
    }
    return true;
}

// -----------------------------------------------------------------------------
// CALLBACK TECLADO
// -----------------------------------------------------------------------------
void processKeys(unsigned char key, int x, int y) {
    switch (key) {
    case 'a': camera.rotateYaw(-0.05f); break;
    case 'd': camera.rotateYaw(0.05f);  break;
    case 'w': camera.rotatePitch(0.05f);
