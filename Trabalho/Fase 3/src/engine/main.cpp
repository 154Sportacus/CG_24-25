#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <cmath>
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
    void rotatePitch(float a) {
        pitch += a;
        if (pitch > M_PI_2 - 0.1f)  pitch = M_PI_2 - 0.1f;
        if (pitch < -M_PI_2 + 0.1f) pitch = -M_PI_2 + 0.1f;
        updatePosition();
    }
    void zoom(float d) { distance += d; updatePosition(); }
};
Camera camera;


static GLfloat gViewMatrix[16];


// Atualiza yaw, pitch e distance a partir de eye, center e up
void updateCameraParameters(Camera& cam) {
    float dx = cam.eye[0] - cam.center[0];
    float dy = cam.eye[1] - cam.center[1];
    float dz = cam.eye[2] - cam.center[2];
    cam.distance = sqrt(dx * dx + dy * dy + dz * dz);
    cam.yaw = atan2(dz, dx);
    cam.pitch = asin(dy / cam.distance);
    cam.updatePosition();
}

// ---------------------------------------------------------------------------
// Estruturas do grafo da cena
// ---------------------------------------------------------------------------
struct Vec3 { float x, y, z; };
enum class TransformType { TRANSLATE, ROTATE, SCALE, TRANSLATE_PATH, ROTATE_TIME };
struct SingleTransform {
    TransformType type;
    Vec3          xyz = { 0,0,0 };
    float         angle = 0.0f;
    float         time = 0.0f;
    bool          align = false;
    vector<Vec3>  path;
};

struct ModelData {
    string fileName;
    Vec3   localTranslation = { 0,0,0 };
    GLuint vbo = 0;
    int    vertexCount = 0;
};

struct SceneNode {
    vector<SingleTransform> transforms;
    vector<ModelData>       models;
    vector<SceneNode>       children;
};

struct Scene {
    vector<SceneNode>         rootNodes;
    map<string, vector<Vec3>> modelLibrary;
} scene;

// -----------------------------------------------------------------------------
// Funções de apoio a Catmull–Rom e vetores
// -----------------------------------------------------------------------------
static void cross(const Vec3& a, const Vec3& b, Vec3& res) {
    res.x = a.y * b.z - a.z * b.y;
    res.y = a.z * b.x - a.x * b.z;
    res.z = a.x * b.y - a.y * b.x;
}
static Vec3 normalize(const Vec3& v) {
    float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len < 1e-6f) return { 0,0,0 };
    return { v.x / len, v.y / len, v.z / len };
}
static void evalCatmullRom(const Vec3& P0, const Vec3& P1,
    const Vec3& P2, const Vec3& P3,
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
// Carrega modelo .3d (gera VBO)
// -----------------------------------------------------------------------------
bool loadModelFile(const string& fname) {
    string path = "../../models/generated/" + fname;
    ifstream in(path);
    if (!in) return false;
    int n; in >> n;
    vector<Vec3> verts(n);
    for (int i = 0; i < n; ++i) in >> verts[i].x >> verts[i].y >> verts[i].z;
    scene.modelLibrary[fname] = verts;
    return true;
}

// -----------------------------------------------------------------------------
// Renderiza o modelo (VBO)
// -----------------------------------------------------------------------------
void renderModel(const ModelData& M) {
    if (M.vbo == 0 || M.vertexCount == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, M.vbo);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vec3), (void*)0);
    glDrawArrays(GL_TRIANGLES, 0, M.vertexCount);
    glDisableClientState(GL_VERTEX_ARRAY);
}

// -----------------------------------------------------------------------------
// Desenha eixos
// -----------------------------------------------------------------------------
void drawAxes() {
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(-15, 0, 0); glVertex3f(15, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, -15, 0); glVertex3f(0, 15, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, -15); glVertex3f(0, 0, 15);
    glEnd();
}

// -----------------------------------------------------------------------------
// Aplica transformações (inclui Catmull–Rom e tempo)
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
            // 1) Draw the static Catmull–Rom loop under *only* the camera
            //    (we push/pop the MODELVIEW to isolate it)
            glPushMatrix();
            glLoadMatrixf(gViewMatrix);   // drop every model transform, keep only the view
            glDisable(GL_DEPTH_TEST);     // so the loop is never occluded
            glColor3f(1, 1, 1);
            glBegin(GL_LINE_LOOP);
            int N = (int)t.path.size();
            for (float U = 0; U < 1.0f; U += 0.01f) {
                float GU = U * N;
                int seg = int(floor(GU)) % N;
                float lt = GU - floor(GU);
                int i0 = (seg - 1 + N) % N,
                    i1 = seg,
                    i2 = (seg + 1) % N,
                    i3 = (seg + 2) % N;
                Vec3 pos, der;
                evalCatmullRom(
                    t.path[i0], t.path[i1],
                    t.path[i2], t.path[i3],
                    lt, pos, der
                );
                glVertex3f(pos.x, pos.y, pos.z);
            }
            glEnd();
            glEnable(GL_DEPTH_TEST);
            glPopMatrix();

            // 2) Now reset to camera only and apply the pure world-space translation+align
            glLoadMatrixf(gViewMatrix);

            float now = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            float Uw = fmodf(now, t.time) / t.time;
            int   Np = (int)t.path.size();
            float GUp = Uw * Np;
            int segp = int(floor(GUp)) % Np;
            float ltp = GUp - floor(GUp);
            int j0 = (segp - 1 + Np) % Np,
                j1 = segp,
                j2 = (segp + 1) % Np,
                j3 = (segp + 2) % Np;
            Vec3 ppos, pder;
            evalCatmullRom(
                t.path[j0], t.path[j1],
                t.path[j2], t.path[j3],
                ltp, ppos, pder
            );

            glTranslatef(ppos.x, ppos.y, ppos.z);
            if (t.align) {
                Vec3 Z = normalize(pder), Y = { 0,1,0 }, X;
                cross(Z, Y, X);
                cross(X, Z, Y);
                float M[16] = {
                  X.x,X.y,X.z,0,  Y.x,Y.y,Y.z,0,
                  Z.x,Z.y,Z.z,0,  0,  0,  0,  1
                };
                glMultMatrixf(M);
            }
        } break;





        case TransformType::ROTATE_TIME: {
            float rv = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            float ang = fmodf(rv, t.time) / t.time * 360.0f;
            glRotatef(ang, t.xyz.x, t.xyz.y, t.xyz.z);
        } break;
        }
    }
}

// -----------------------------------------------------------------------------
// Desenha cena recursivamente
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

    glGetFloatv(GL_MODELVIEW_MATRIX, gViewMatrix);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    drawAxes();
    for (auto& node : scene.rootNodes) drawSceneNode(node);
    glutSwapBuffers();
}

// -----------------------------------------------------------------------------
// Change Size
// -----------------------------------------------------------------------------
void changeSize(int w, int h) {
    if (!h) h = 1;
    float ratio = (float)w / (float)h;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);
    gluPerspective(45.0f, ratio, 1.0f, 1000.0f);
    glMatrixMode(GL_MODELVIEW);
}

// -----------------------------------------------------------------------------
// Parse XML completo (window + camera + cena)
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// Função recursiva para parse de <group>
// -----------------------------------------------------------------------------
bool parseGroup(XMLElement* g, SceneNode& node) {
    // parse <transform>
    if (XMLElement* tr = g->FirstChildElement("transform")) {
        for (XMLElement* t = tr->FirstChildElement(); t; t = t->NextSiblingElement()) {
            string name = t->Name(); SingleTransform st;
            if (name == "translate") {
                if (t->Attribute("time")) {
                    st.type = TransformType::TRANSLATE_PATH;
                    st.time = t->FloatAttribute("time");
                    st.align = t->BoolAttribute("align");
                    for (XMLElement* pt = t->FirstChildElement("point"); pt; pt = pt->NextSiblingElement("point"))
                        st.path.push_back({ pt->FloatAttribute("x"), pt->FloatAttribute("y"), pt->FloatAttribute("z") });
                }
                else {
                    st.type = TransformType::TRANSLATE;
                    st.xyz = { t->FloatAttribute("x"), t->FloatAttribute("y"), t->FloatAttribute("z") };
                }
            }
            else if (name == "rotate") {
                if (t->Attribute("time")) {
                    st.type = TransformType::ROTATE_TIME;
                    st.time = t->FloatAttribute("time");
                    st.xyz = { t->FloatAttribute("x"), t->FloatAttribute("y"), t->FloatAttribute("z") };
                }
                else {
                    st.type = TransformType::ROTATE;
                    st.angle = t->FloatAttribute("angle");
                    st.xyz = { t->FloatAttribute("x"), t->FloatAttribute("y"), t->FloatAttribute("z") };
                }
            }
            else if (name == "scale") {
                st.type = TransformType::SCALE;
                st.xyz = { t->FloatAttribute("x"), t->FloatAttribute("y"), t->FloatAttribute("z") };
            }
            node.transforms.push_back(st);
        }
    }
    // parse <models>
    if (XMLElement* ms = g->FirstChildElement("models")) {
        for (XMLElement* m = ms->FirstChildElement("model"); m; m = m->NextSiblingElement("model")) {
            ModelData md;
            const char* f = m->Attribute("file");
            if (f) {
                md.fileName = f;
                if (!scene.modelLibrary.count(md.fileName)) {
                    if (!loadModelFile(md.fileName)) return false;
                }
                auto& verts = scene.modelLibrary[md.fileName];
                glGenBuffers(1, &md.vbo);
                glBindBuffer(GL_ARRAY_BUFFER, md.vbo);
                glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vec3), verts.data(), GL_STATIC_DRAW);
                md.vertexCount = (int)verts.size();
                node.models.push_back(md);
            }
        }
    }
    // parse children
    for (XMLElement* c = g->FirstChildElement("group"); c; c = c->NextSiblingElement("group")) {
        SceneNode child;
        if (!parseGroup(c, child)) return false;
        node.children.push_back(child);
    }
    return true;
}

bool parseXML(const char* file) {
    XMLDocument doc;
    if (doc.LoadFile(file) != XML_SUCCESS) return false;
    XMLElement* w = doc.FirstChildElement("world");
    if (!w) return false;

    for (XMLElement* g = w->FirstChildElement("group");g;g = g->NextSiblingElement("group")) {
        SceneNode rn;
        if (!parseGroup(g, rn)) return false;
        scene.rootNodes.push_back(rn);
    }
    return true;
}

// ------------------------------------------------------------------
// Window e Camera parser
// ------------------------------------------------------------------
bool parseWindowAndCamera(const char* file) {
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(file) != XML_SUCCESS) return false;
    auto* w = doc.FirstChildElement("world");
    if (!w) return false;

    // window
    if (auto* win = w->FirstChildElement("window")) {
        win->QueryIntAttribute("width", &gWindowWidth);
        win->QueryIntAttribute("height", &gWindowHeight);
    }

    // camera (same as you already do in parseXML)
    if (auto* cam = w->FirstChildElement("camera")) {
        if (auto* p = cam->FirstChildElement("position")) {
            p->QueryFloatAttribute("x", &camera.eye[0]);
            p->QueryFloatAttribute("y", &camera.eye[1]);
            p->QueryFloatAttribute("z", &camera.eye[2]);
        }
        if (auto* la = cam->FirstChildElement("lookAt")) {
            la->QueryFloatAttribute("x", &camera.center[0]);
            la->QueryFloatAttribute("y", &camera.center[1]);
            la->QueryFloatAttribute("z", &camera.center[2]);
        }
        if (auto* up = cam->FirstChildElement("up")) {
            up->QueryFloatAttribute("x", &camera.up[0]);
            up->QueryFloatAttribute("y", &camera.up[1]);
            up->QueryFloatAttribute("z", &camera.up[2]);
        }
        updateCameraParameters(camera);
    }

    return true;
}




// -----------------------------------------------------------------------------
// Callback teclado
// -----------------------------------------------------------------------------
void processKeys(unsigned char key, int x, int y) {
    switch (key) {
    case 'a': camera.rotateYaw(-0.05f); break;
    case 'd': camera.rotateYaw(0.05f); break;
    case 'w': camera.rotatePitch(0.05f); break;
    case 's': camera.rotatePitch(-0.05f); break;
    case 'x': camera.zoom(-0.3f);         break;
    case 'z': camera.zoom(0.3f);         break;
    case 27: exit(0);                    break;
    }
    glutPostRedisplay();
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------


int main(int argc, char** argv) {
    const char* xmlFile = "../../engine/inputs/test_3_1.xml";

    if (!parseWindowAndCamera(xmlFile)) {
        std::cerr << "Erro ao ler window/camera do XML\n";
        return 1;
    }

  
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(gWindowWidth, gWindowHeight);
    glutCreateWindow("Engine 3D - Phase 3");
  
    
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        cerr << "GLEW error: " << glewGetErrorString(err) << endl;
        return -1;
    }

    // First parse the XML so we pick up <window width=… height=…>
    if (!parseXML(xmlFile)) {
        cerr << "Erro ao ler XML" << endl;
        return 1;

    }


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);
    glutKeyboardFunc(processKeys);
    glutIdleFunc(renderScene);
    glutMainLoop();
    return 0;
}






   