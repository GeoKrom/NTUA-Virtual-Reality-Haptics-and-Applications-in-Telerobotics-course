// ============================================================
// ROBOTIC ARM - OpenGL + GLUT
// Linux / C++
// ============================================================
//
// Controls:
//
// A / D       : Περιστροφή βάσης
// W / S       : Κίνηση ώμου
// Q / E       : Κίνηση αγκώνα
// Z / X       : Άνοιγμα / κλείσιμο δακτύλων
// P           : Σύλληψη αντικειμένου
// L           : Απελευθέρωση αντικειμένου
// R           : Reset
//
// Arrow Keys  : Περιστροφή κάμερας
// + / -       : Zoom
// ESC         : Έξοδος
//
// ============================================================

#include <GL/glut.h>
#include <GL/glu.h>
#include <cmath>
#include <cstdlib>

// ============================================================
// CONSTANTS
// ============================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================
// OBJECT STRUCTURE
// ============================================================

struct Object3D
{
    float x;
    float y;
    float z;

    float radius;

    bool grabbed;
    bool isSphere;
};

// ============================================================
// ROBOT PARAMETERS
// ============================================================

// Base
float baseHeight = 1.0f;
float baseRadius = 1.2f;

// Arm lengths
float L1 = 3.0f;
float L2 = 2.5f;

// Joint angles
//
// The arm starts in an elevated configuration.
//
float baseAngle = 0.0f;
float shoulderAngle = 55.0f;
float elbowAngle = -15.0f;

// ============================================================
// GRIPPER PARAMETERS
// ============================================================

float gripperOpening = 0.55f;

float fingerLength1 = 0.45f;
float fingerLength2 = 0.40f;

int grabbedObject = -1;

// ============================================================
// OBJECTS
// ============================================================

Object3D sphere =
{
    3.5f,
    0.0f,
    0.75f,
    0.75f,
    false,
    true
};

Object3D cylinder =
{
    -3.0f,
    1.5f,
    0.8f,
    0.7f,
    false,
    false
};

// ============================================================
// CAMERA
// ============================================================

float cameraDistance = 15.0f;
float cameraYaw = 45.0f;
float cameraPitch = 25.0f;

// ============================================================
// WINDOW
// ============================================================

int windowWidth = 1000;
int windowHeight = 700;

// ============================================================
// MATERIAL / COLOR
// ============================================================

void setColor(float r, float g, float b){
    
    GLfloat color[] =
    {
        r, g, b, 1.0f
    };

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE,
        color
    );
}

// ============================================================
// DRAW CYLINDER
// ============================================================

void drawCylinder(float radius, float height)
{
    GLUquadric* quadric = gluNewQuadric();

    if (!quadric)
        return;

    gluCylinder(
        quadric,
        radius,
        radius,
        height,
        32,
        8
    );

    // Bottom cap
    glPushMatrix();

    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);

    gluDisk(
        quadric,
        0.0,
        radius,
        32,
        1
    );

    glPopMatrix();

    // Top cap
    glPushMatrix();

    glTranslatef(0.0f, 0.0f, height);

    gluDisk(
        quadric,
        0.0,
        radius,
        32,
        1
    );

    glPopMatrix();

    gluDeleteQuadric(quadric);
}

// ============================================================
// DRAW SPHERE
// ============================================================

void drawSphere(float radius)
{
    glutSolidSphere(
        radius,
        32,
        32
    );
}

// ============================================================
// DRAW CUBE
// ============================================================

void drawCube(
    float sx,
    float sy,
    float sz
)
{
    glPushMatrix();

    glScalef(
        sx,
        sy,
        sz
    );

    glutSolidCube(1.0f);

    glPopMatrix();
}

// ============================================================
// DRAW FLOOR
// ============================================================

void drawFloor()
{
    setColor(
        0.12f,
        0.14f,
        0.16f
    );

    glBegin(GL_QUADS);

    glNormal3f(
        0.0f,
        0.0f,
        1.0f
    );

    glVertex3f(
        -10.0f,
        -10.0f,
        0.0f
    );

    glVertex3f(
        10.0f,
        -10.0f,
        0.0f
    );

    glVertex3f(
        10.0f,
        10.0f,
        0.0f
    );

    glVertex3f(
        -10.0f,
        10.0f,
        0.0f
    );

    glEnd();
}

// ============================================================
// DRAW GRID
// ============================================================

void drawGrid()
{
    glDisable(GL_LIGHTING);

    glLineWidth(1.0f);

    glColor3f(
        0.25f,
        0.27f,
        0.29f
    );

    glBegin(GL_LINES);

    for (int i = -10; i <= 10; i++)
    {
        glVertex3f(
            (float)i,
            -10.0f,
            0.01f
        );

        glVertex3f(
            (float)i,
            10.0f,
            0.01f
        );

        glVertex3f(
            -10.0f,
            (float)i,
            0.01f
        );

        glVertex3f(
            10.0f,
            (float)i,
            0.01f
        );
    }

    glEnd();

    glEnable(GL_LIGHTING);
}

// ============================================================
// DRAW JOINT
// ============================================================

void drawJoint(float radius)
{
    setColor(
        0.75f,
        0.75f,
        0.78f
    );

    glutSolidSphere(
        radius,
        24,
        24
    );
}

// ============================================================
// DRAW ROBOT BASE
// ============================================================

void drawRobotBase()
{
    setColor(
        0.20f,
        0.32f,
        0.48f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        0.0f,
        baseHeight / 2.0f
    );

    drawCylinder(
        baseRadius,
        baseHeight
    );

    glPopMatrix();

    // Rotating upper platform
    setColor(
        0.28f,
        0.42f,
        0.62f
    );

    glPushMatrix();

    glTranslatef(
        0.0f,
        0.0f,
        baseHeight
    );

    drawCylinder(
        0.85f,
        0.35f
    );

    glPopMatrix();
}

// ============================================================
// DRAW UPPER ARM
// ============================================================

void drawUpperArm()
{
    // Η συνάρτηση καλείται ήδη από τη θέση του ώμου
    // και η περιστροφή του ώμου έχει εφαρμοστεί στο drawRobot().

    glPushMatrix();

    // Το κέντρο του upper arm βρίσκεται στο L1/2
    glTranslatef(
        L1 / 2.0f,
        0.0f,
        0.0f
    );

    setColor(
        0.65f,
        0.68f,
        0.72f
    );

    drawCube(
        L1,
        0.65f,
        0.65f
    );

    glPopMatrix();
}

void drawForearm()
{
    // Η συνάρτηση καλείται ήδη από τη θέση του αγκώνα
    // και η γωνία του αγκώνα έχει εφαρμοστεί στο drawRobot().

    glPushMatrix();

    // Το κέντρο του forearm βρίσκεται στο L2/2
    glTranslatef(
        L2 / 2.0f,
        0.0f,
        0.0f
    );

    setColor(
        0.55f,
        0.58f,
        0.62f
    );

    drawCube(
        L2,
        0.55f,
        0.55f
    );

    glPopMatrix();
}
// ============================================================
// DRAW FINGER
// ============================================================

void drawFinger(float yOffset)
{
    glPushMatrix();

    // --------------------------------------------------------
    // Το finger ξεκινά ακριβώς από το άκρο του forearm.
    // Δεν κάνουμε επιπλέον μετατόπιση κατά L2.
    // --------------------------------------------------------

    glTranslatef(
        0.0f,
        yOffset,
        0.0f
    );

    // ========================================================
    // ΠΡΩΤΗ ΦΑΛΑΓΓΑ
    // ========================================================

    glPushMatrix();

    glTranslatef(
        0.22f,
        0.0f,
        0.0f
    );

    setColor(
        0.72f,
        0.75f,
        0.80f
    );

    drawCube(
        0.44f,
        0.16f,
        0.16f
    );

    glPopMatrix();

    // ========================================================
    // ΔΕΥΤΕΡΗ ΦΑΛΑΓΓΑ
    // ========================================================

    glPushMatrix();

    glTranslatef(
        0.52f,
        0.0f,
        0.0f
    );

    glRotatef(
        15.0f,
        0.0f,
        1.0f,
        0.0f
    );

    drawCube(
        0.38f,
        0.14f,
        0.14f
    );

    glPopMatrix();

    // ========================================================
    // ΑΚΡΟ ΔΑΚΤΥΛΟΥ
    // ========================================================

    glPushMatrix();

    glTranslatef(
        0.76f,
        0.0f,
        0.0f
    );

    setColor(
        0.90f,
        0.55f,
        0.15f
    );

    drawCube(
        0.18f,
        0.20f,
        0.20f
    );

    glPopMatrix();

    glPopMatrix();
}
// ============================================================
// DRAW GRIPPER
// ============================================================

void drawGripper()
{
    glPushMatrix();

    // Το gripper βρίσκεται ΗΔΗ στο άκρο του forearm.
    // Δεν κάνουμε επιπλέον μετατόπιση κατά L2.

    // ---------------------------------------------
    // Palm / βάση δακτύλων
    // ---------------------------------------------

    setColor(
        0.35f,
        0.40f,
        0.48f
    );

    glPushMatrix();

    glTranslatef(
        0.15f,
        0.0f,
        0.0f
    );

    drawCube(
        0.30f,
        1.10f,
        0.35f
    );

    glPopMatrix();

    // ---------------------------------------------
    // Αριστερό δάκτυλο
    // ---------------------------------------------

    drawFinger(
        gripperOpening
    );

    // ---------------------------------------------
    // Δεξί δάκτυλο
    // ---------------------------------------------

    drawFinger(
        -gripperOpening
    );

    glPopMatrix();
}
// ============================================================
// CALCULATE END EFFECTOR
// ============================================================

void calculateEndEffector(
    float& x,
    float& y,
    float& z
)
{
    float theta1 =
        shoulderAngle * M_PI / 180.0f;

    float theta2 =
        elbowAngle * M_PI / 180.0f;

    float base =
        baseAngle * M_PI / 180.0f;

    // Συνολική οριζόντια απόσταση
    float horizontal =
        L1 * cos(theta1)
        +
        L2 * cos(theta1 + theta2);

    // Ύψος end-effector
    z =
        baseHeight
        + 0.35f
        +
        L1 * sin(theta1)
        +
        L2 * sin(theta1 + theta2);

    // Περιστροφή βάσης
    x =
        horizontal * cos(base);

    y =
        horizontal * sin(base);
}

// ============================================================
// DRAW SPHERE OBJECT
// ============================================================

void drawSphereObject()
{
    setColor(
        0.85f,
        0.15f,
        0.12f
    );

    glPushMatrix();

    glTranslatef(
        sphere.x,
        sphere.y,
        sphere.z
    );

    drawSphere(
        sphere.radius
    );

    glPopMatrix();
}

// ============================================================
// DRAW CYLINDER OBJECT
// ============================================================

void drawCylinderObject()
{
    setColor(
        0.15f,
        0.70f,
        0.30f
    );

    glPushMatrix();

    glTranslatef(
        cylinder.x,
        cylinder.y,
        cylinder.z
    );

    drawCylinder(
        cylinder.radius,
        1.6f
    );

    glPopMatrix();
}

// ============================================================
// DRAW OBJECTS
// ============================================================

void drawObjects()
{
    drawSphereObject();
    drawCylinderObject();
}

// ============================================================
// DISTANCE TO OBJECT
// ============================================================

float distanceToObject(
    float x,
    float y,
    float z,
    const Object3D& object
)
{
    float dx =
        x - object.x;

    float dy =
        y - object.y;

    float dz =
        z - object.z;

    return sqrt(
        dx * dx +
        dy * dy +
        dz * dz
    );
}

// ============================================================
// TRY GRAB
// ============================================================

void tryGrab()
{
    float endX;
    float endY;
    float endZ;

    calculateEndEffector(
        endX,
        endY,
        endZ
    );

    const float grabDistance = 1.40f;

    // Check sphere
    if (!sphere.grabbed)
    {
        float d =
            distanceToObject(
                endX,
                endY,
                endZ,
                sphere
            );

        if (d < grabDistance)
        {
            sphere.grabbed = true;
            grabbedObject = 0;
            return;
        }
    }

    // Check cylinder
    if (!cylinder.grabbed)
    {
        float d =
            distanceToObject(
                endX,
                endY,
                endZ,
                cylinder
            );

        if (d < grabDistance)
        {
            cylinder.grabbed = true;
            grabbedObject = 1;
            return;
        }
    }
}

// ============================================================
// UPDATE GRABBED OBJECT
// ============================================================

void updateGrabbedObject()
{
    if (grabbedObject == -1)
        return;

    float endX;
    float endY;
    float endZ;

    calculateEndEffector(
        endX,
        endY,
        endZ
    );

    if (grabbedObject == 0)
    {
        sphere.x = endX;
        sphere.y = endY;
        sphere.z = endZ - 0.65f;
    }
    else if (grabbedObject == 1)
    {
        cylinder.x = endX;
        cylinder.y = endY;
        cylinder.z = endZ - 0.80f;
    }
}

// ============================================================
// RELEASE OBJECT
// ============================================================

void releaseGrabbedObject()
{
    if (grabbedObject == 0)
    {
        sphere.grabbed = false;
    }
    else if (grabbedObject == 1)
    {
        cylinder.grabbed = false;
    }

    grabbedObject = -1;
}

// ============================================================
// RESET ROBOT
// ============================================================

void resetRobot()
{
    // Elevated initial configuration
    baseAngle = 0.0f;

    shoulderAngle = 55.0f;

    elbowAngle = -15.0f;

    gripperOpening = 0.55f;

    grabbedObject = -1;

    sphere.grabbed = false;
    cylinder.grabbed = false;

    // Reset sphere
    sphere.x = 3.5f;
    sphere.y = 0.0f;
    sphere.z = 0.75f;

    // Reset cylinder
    cylinder.x = -3.0f;
    cylinder.y = 1.5f;
    cylinder.z = 0.8f;
}

// ============================================================
// DRAW ROBOT
// ============================================================

void drawRobot()
{
    glPushMatrix();

    // ========================================================
    // BASE
    // ========================================================

    glRotatef(
        baseAngle,
        0.0f,
        0.0f,
        1.0f
    );

    drawRobotBase();

    // ========================================================
    // MOVE TO SHOULDER
    // ========================================================

    glTranslatef(
        0.0f,
        0.0f,
        baseHeight + 0.35f
    );

    // Shoulder joint
    drawJoint(0.40f);

    // ========================================================
    // SHOULDER ROTATION
    // ========================================================

    glRotatef(
        -shoulderAngle,
        0.0f,
        1.0f,
        0.0f
    );

    // ========================================================
    // UPPER ARM
    // ========================================================

    drawUpperArm();

    // ========================================================
    // MOVE TO ELBOW
    // ========================================================

    glTranslatef(
        L1,
        0.0f,
        0.0f
    );

    // Elbow joint
    drawJoint(0.32f);

    // ========================================================
    // ELBOW ROTATION
    // ========================================================

    glRotatef(
        -elbowAngle,
        0.0f,
        1.0f,
        0.0f
    );

    // ========================================================
    // FOREARM
    // ========================================================

    drawForearm();

    // ========================================================
    // MOVE TO END OF FOREARM
    // ========================================================

    glTranslatef(
        L2,
        0.0f,
        0.0f
    );

    // ========================================================
    // GRIPPER
    // ========================================================

    drawGripper();

    glPopMatrix();
}

// ============================================================
// LIGHTING
// ============================================================

void setupLighting()
{
    glEnable(GL_LIGHTING);

    glEnable(GL_LIGHT0);

    GLfloat ambient[] =
    {
        0.35f,
        0.35f,
        0.35f,
        1.0f
    };

    GLfloat diffuse[] =
    {
        0.95f,
        0.95f,
        0.95f,
        1.0f
    };

    GLfloat specular[] =
    {
        1.0f,
        1.0f,
        1.0f,
        1.0f
    };

    GLfloat position[] =
    {
        4.0f,
        5.0f,
        10.0f,
        1.0f
    };

    glLightfv(
        GL_LIGHT0,
        GL_AMBIENT,
        ambient
    );

    glLightfv(
        GL_LIGHT0,
        GL_DIFFUSE,
        diffuse
    );

    glLightfv(
        GL_LIGHT0,
        GL_SPECULAR,
        specular
    );

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        position
    );

    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE
    );

    glShadeModel(
        GL_SMOOTH
    );
}

// ============================================================
// CAMERA
// ============================================================

void setupCamera()
{
    glMatrixMode(
        GL_MODELVIEW
    );

    glLoadIdentity();

    float yawRad =
        cameraYaw * M_PI / 180.0f;

    float pitchRad =
        cameraPitch * M_PI / 180.0f;

    float camX =
        cameraDistance
        *
        cos(pitchRad)
        *
        sin(yawRad);

    float camY =
        cameraDistance
        *
        cos(pitchRad)
        *
        cos(yawRad);

    float camZ =
        cameraDistance
        *
        sin(pitchRad);

    gluLookAt(
        camX,
        camY,
        camZ,

        0.0f,
        0.0f,
        2.0f,

        0.0f,
        0.0f,
        1.0f
    );
}

// ============================================================
// DISPLAY
// ============================================================

void display()
{
    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    setupCamera();

    // Floor
    drawFloor();

    // Grid
    drawGrid();

    // Objects
    drawObjects();

    // Robot
    drawRobot();

    glutSwapBuffers();
}

// ============================================================
// RESHAPE
// ============================================================

void reshape(
    int width,
    int height
)
{
    if (height == 0)
        height = 1;

    windowWidth = width;
    windowHeight = height;

    glViewport(
        0,
        0,
        width,
        height
    );

    glMatrixMode(
        GL_PROJECTION
    );

    glLoadIdentity();

    gluPerspective(
        45.0,
        (double)width / (double)height,
        0.1,
        100.0
    );

    glMatrixMode(
        GL_MODELVIEW
    );
}

// ============================================================
// KEYBOARD
// ============================================================

void keyboard(
    unsigned char key,
    int x,
    int y
)
{
    switch (key)
    {
        // ----------------------------------------------------
        // BASE
        // ----------------------------------------------------

        case 'a':
        case 'A':
            baseAngle += 5.0f;
            break;

        case 'd':
        case 'D':
            baseAngle -= 5.0f;
            break;

        // ----------------------------------------------------
        // SHOULDER
        // ----------------------------------------------------

        case 'w':
        case 'W':
            shoulderAngle += 3.0f;
            break;

        case 's':
        case 'S':
            shoulderAngle -= 3.0f;
            break;

        // ----------------------------------------------------
        // ELBOW
        // ----------------------------------------------------

        case 'q':
        case 'Q':
            elbowAngle += 3.0f;
            break;

        case 'e':
        case 'E':
            elbowAngle -= 3.0f;
            break;

        // ----------------------------------------------------
        // GRIPPER
        // ----------------------------------------------------

        case 'z':
        case 'Z':

            gripperOpening += 0.05f;

            if (gripperOpening > 1.0f)
                gripperOpening = 1.0f;

            break;

        case 'x':
        case 'X':

            gripperOpening -= 0.05f;

            if (gripperOpening < 0.15f)
                gripperOpening = 0.15f;

            break;

        // ----------------------------------------------------
        // GRAB
        // ----------------------------------------------------

        case 'p':
        case 'P':
            tryGrab();
            break;

        // ----------------------------------------------------
        // RELEASE
        // ----------------------------------------------------

        case 'l':
        case 'L':
            releaseGrabbedObject();
            break;

        // ----------------------------------------------------
        // RESET
        // ----------------------------------------------------

        case 'r':
        case 'R':
            resetRobot();
            break;

        // ----------------------------------------------------
        // ZOOM
        // ----------------------------------------------------

        case '+':
        case '=':

            cameraDistance -= 0.5f;

            if (cameraDistance < 5.0f)
                cameraDistance = 5.0f;

            break;

        case '-':
        case '_':

            cameraDistance += 0.5f;

            if (cameraDistance > 30.0f)
                cameraDistance = 30.0f;

            break;

        // ----------------------------------------------------
        // EXIT
        // ----------------------------------------------------

        case 27:
            std::exit(0);
            break;
    }

    glutPostRedisplay();
}

// ============================================================
// SPECIAL KEYS
// ============================================================

void specialKeys(
    int key,
    int x,
    int y
)
{
    switch (key)
    {
        case GLUT_KEY_LEFT:
            cameraYaw -= 5.0f;
            break;

        case GLUT_KEY_RIGHT:
            cameraYaw += 5.0f;
            break;

        case GLUT_KEY_UP:
            cameraPitch += 3.0f;

            if (cameraPitch > 75.0f)
                cameraPitch = 75.0f;

            break;

        case GLUT_KEY_DOWN:
            cameraPitch -= 3.0f;

            if (cameraPitch < 5.0f)
                cameraPitch = 5.0f;

            break;
    }

    glutPostRedisplay();
}

// ============================================================
// UPDATE
// ============================================================

void update()
{
    updateGrabbedObject();

    glutPostRedisplay();
}

// ============================================================
// TIMER
// ============================================================

void timer(int value)
{
    update();

    glutTimerFunc(
        16,
        timer,
        0
    );
}

// ============================================================
// OPENGL INITIALIZATION
// ============================================================

void initOpenGL()
{
    glClearColor(
        0.025f,
        0.035f,
        0.045f,
        1.0f
    );

    glEnable(
        GL_DEPTH_TEST
    );

    glEnable(
        GL_NORMALIZE
    );

    glEnable(
        GL_CULL_FACE
    );

    glCullFace(
        GL_BACK
    );

    glEnable(
        GL_COLOR_MATERIAL
    );

    setupLighting();
}

// ============================================================
// MAIN
// ============================================================

int main(
    int argc,
    char** argv
)
{
    // --------------------------------------------------------
    // GLUT INITIALIZATION
    // --------------------------------------------------------

    glutInit(
        &argc,
        argv
    );

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(
        windowWidth,
        windowHeight
    );

    glutInitWindowPosition(
        100,
        50
    );

    glutCreateWindow(
        "OpenGL Robotic Arm - Two Finger Gripper"
    );

    // --------------------------------------------------------
    // OPENGL INITIALIZATION
    // --------------------------------------------------------

    initOpenGL();

    // --------------------------------------------------------
    // CALLBACK FUNCTIONS
    // --------------------------------------------------------

    glutDisplayFunc(
        display
    );

    glutReshapeFunc(
        reshape
    );

    glutKeyboardFunc(
        keyboard
    );

    glutSpecialFunc(
        specialKeys
    );

    glutTimerFunc(
        16,
        timer,
        0
    );

    // --------------------------------------------------------
    // START MAIN LOOP
    // --------------------------------------------------------

    glutMainLoop();

    return 0;
}
