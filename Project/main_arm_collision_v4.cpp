// ============================================================
// ROBOTIC ARM WITH PHYSICS
// OpenGL + GLUT
// Linux / C++
//
// CONTROLS
// ------------------------------------------------------------
// A / D       : Rotate robot base
// W / S       : Move shoulder
// Q / E       : Move elbow
// Z           : Open gripper
// X           : Close gripper
// P           : Try to grab object
// L           : Release object
// R           : Reset simulation
// Arrow Keys  : Camera rotation
// + / -       : Camera zoom
// F           : Toggle force visualization
// ESC         : Exit
// ============================================================

#include <GL/glut.h>
#include <GL/glu.h>

#include <cmath>
#include <cstdlib>
#include <algorithm>

// ============================================================
// CONSTANTS
// ============================================================

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const float GRAVITY = 9.81f;
const float PHYSICS_DT = 0.016f;

// ============================================================
// FUNCTION PROTOTYPES
// ============================================================

void resetSimulation();
void updatePhysics();
void display();
void reshape(int width, int height);

void keyboard(
    unsigned char key,
    int x,
    int y
);

void specialKeys(
    int key,
    int x,
    int y
);

void timer(int value);

// ============================================================
// OBJECT TYPES
// ============================================================

enum ObjectType
{
    SPHERE_OBJECT,
    CYLINDER_OBJECT
};

// ============================================================
// OBJECT STRUCTURE
// ============================================================

struct Object3D
{
    float x, y, z;

    float vx, vy, vz;

    float fx, fy, fz;

    float mass;

    float radius;

    float height;

    float restitution;

    float friction;

    bool grabbed;

    ObjectType type;
};

// ============================================================
// ROBOT PARAMETERS
// ============================================================

float baseHeight = 1.0f;
float baseRadius = 1.2f;

float L1 = 3.0f;
float L2 = 2.5f;

// ============================================================
// INITIAL ROBOT CONFIGURATION
// ============================================================

float baseAngle = 0.0f;

// Elevated initial configuration
float shoulderAngle = 55.0f;
float elbowAngle = -15.0f;

// ============================================================
// GRIPPER PARAMETERS
// ============================================================

// Distance of each finger from the gripper centerline.
// The two fingers therefore remain mechanically attached to the palm.
float gripperOpening = 0.88f;

float minimumOpening = 0.12f;
// Half-separation must accommodate the sphere/cylinder radius plus fingertip clearance.
float maximumOpening = 0.90f;

float maxGripForce = 120.0f;

float gripStiffness = 240.0f;
float gripDamping = 22.0f;

// ============================================================
// OBJECTS
// ============================================================

Object3D sphere =
{
    4.0f,
    -1.8f,
    0.75f,

    0.0f,
    0.0f,
    0.0f,

    0.0f,
    0.0f,
    0.0f,

    1.0f,
    0.75f,
    1.5f,

    0.58f,
    0.55f,

    false,

    SPHERE_OBJECT
};

Object3D cylinder =
{
    -4.0f,
    2.2f,
    0.80f,

    0.0f,
    0.0f,
    0.0f,

    0.0f,
    0.0f,
    0.0f,

    1.5f,
    0.70f,
    1.60f,

    0.42f,
    0.60f,

    false,

    CYLINDER_OBJECT
};

// ============================================================
// GRABBED OBJECT
// ============================================================

int grabbedObject = -1;

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
// FORCE VISUALIZATION
// ============================================================

bool showForces = true;

// ============================================================
// UTILITY
// ============================================================

float clampValue(
    float value,
    float minValue,
    float maxValue
)
{
    return std::max(
        minValue,
        std::min(
            value,
            maxValue
        )
    );
}

// ============================================================
// VECTOR MAGNITUDE
// ============================================================

float vectorMagnitude(
    float x,
    float y,
    float z
)
{
    return std::sqrt(
        x * x +
        y * y +
        z * z
    );
}

// ============================================================
// MATERIAL COLOR
// ============================================================

void setColor(
    float r,
    float g,
    float b
)
{
    GLfloat color[] =
    {
        r,
        g,
        b,
        1.0f
    };

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE,
        color
    );
}

// ============================================================
// CUBE
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
// CYLINDER
// ============================================================

void drawCylinder(
    float radius,
    float height
)
{
    GLUquadric* quadric =
        gluNewQuadric();

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

    glPushMatrix();

    glRotatef(
        180.0f,
        1.0f,
        0.0f,
        0.0f
    );

    gluDisk(
        quadric,
        0.0,
        radius,
        32,
        1
    );

    glPopMatrix();

    glPushMatrix();

    glTranslatef(
        0.0f,
        0.0f,
        height
    );

    gluDisk(
        quadric,
        0.0,
        radius,
        32,
        1
    );

    glPopMatrix();

    gluDeleteQuadric(
        quadric
    );
}

// ============================================================
// SPHERE
// ============================================================

void drawSphere(
    float radius
)
{
    glutSolidSphere(
        radius,
        32,
        32
    );
}

// ============================================================
// FLOOR
// ============================================================

void drawFloor()
{
    setColor(
        0.13f,
        0.15f,
        0.17f
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
// GRID
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

    for (int i = -10; i <= 10; ++i)
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
// JOINT
// ============================================================

void drawJoint(
    float radius
)
{
    setColor(
        0.80f,
        0.82f,
        0.85f
    );

    glutSolidSphere(
        radius,
        24,
        24
    );
}

// ============================================================
// ROBOT BASE
// ============================================================

void drawRobotBase()
{
    setColor(
        0.20f,
        0.34f,
        0.52f
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

    setColor(
        0.28f,
        0.43f,
        0.64f
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
// UPPER ARM
// ============================================================

void drawUpperArm()
{
    glPushMatrix();

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

    // Thicker upper arm for a more robust mechanical appearance.
    drawCube(
        L1,
        0.95f,
        0.95f
    );

    glPopMatrix();
}

// ============================================================
// FOREARM
// ============================================================

void drawForearm()
{
    glPushMatrix();

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

    // Thicker forearm, while keeping the same length and kinematics.
    drawCube(
        L2,
        0.80f,
        0.80f
    );

    glPopMatrix();
}

// ============================================================
// FINGER
// ============================================================

void drawFinger(
    float yOffset
)
{
    // Mechanical travel limit:
    // each finger can only move within the palm width.
    // This prevents the fingers from escaping into free space.
    yOffset = clampValue(
        yOffset,
        -maximumOpening,
        maximumOpening
    );

    glPushMatrix();

    // Lateral position of the finger.
    // The finger remains attached to the gripper palm.
    glTranslatef(
        0.0f,
        yOffset,
        0.0f
    );

    // First phalange
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

    // Second phalange
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

    // Fingertip
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
// GRIPPER
// ============================================================

void drawGripper()
{
    glPushMatrix();

    // The coordinate system is already at the
    // end of the forearm.

    // Palm
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
        2.00f,
        0.35f
    );

    glPopMatrix();

    // Left finger
    drawFinger(
        gripperOpening
    );

    // Right finger
    drawFinger(
        -gripperOpening
    );

    glPopMatrix();
}

// ============================================================
// END EFFECTOR KINEMATICS
// ============================================================

void calculateEndEffector(
    float& x,
    float& y,
    float& z
)
{
    float theta1 =
        shoulderAngle *
        M_PI /
        180.0f;

    float theta2 =
        elbowAngle *
        M_PI /
        180.0f;

    float base =
        baseAngle *
        M_PI /
        180.0f;

    float horizontal =
        L1 * std::cos(theta1)
        +
        L2 * std::cos(theta1 + theta2);

    z =
        baseHeight
        +
        0.35f
        +
        L1 * std::sin(theta1)
        +
        L2 * std::sin(theta1 + theta2);

    x =
        horizontal *
        std::cos(base);

    y =
        horizontal *
        std::sin(base);
}

// ============================================================
// END EFFECTOR DIRECTION
// ============================================================

void calculateEndEffectorDirection(
    float& dx,
    float& dy,
    float& dz
)
{
    float theta1 =
        shoulderAngle *
        M_PI /
        180.0f;

    float theta2 =
        elbowAngle *
        M_PI /
        180.0f;

    float base =
        baseAngle *
        M_PI /
        180.0f;

    float angle =
        theta1 + theta2;

    float localX =
        std::cos(angle);

    float localZ =
        std::sin(angle);

    dx =
        localX *
        std::cos(base);

    dy =
        localX *
        std::sin(base);

    dz =
        localZ;
}

// ============================================================
// OBJECT DRAWING
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
        cylinder.z -
        cylinder.height / 2.0f
    );

    drawCylinder(
        cylinder.radius,
        cylinder.height
    );

    glPopMatrix();
}

// ============================================================

void drawObjects()
{
    drawSphereObject();

    drawCylinderObject();
}

// ============================================================
// PHYSICS: GRAVITY
// ============================================================

void applyGravity(
    Object3D& object
)
{
    object.fz -=
        object.mass *
        GRAVITY;
}

// ============================================================
// PHYSICS: FLOOR CONTACT
// ============================================================
// The floor is treated as a rigid unilateral constraint:
//
//     z_bottom >= 0
//
// A penalty force is generated during penetration, while a
// post-integration constraint correction guarantees that the
// object can never remain below the floor.
// ============================================================

float getObjectBottom(const Object3D& object)
{
    if (object.type == SPHERE_OBJECT)
    {
        return object.z - object.radius;
    }

    return object.z - object.height / 2.0f;
}

float getObjectFloorHeight(const Object3D& object)
{
    if (object.type == SPHERE_OBJECT)
    {
        return object.radius;
    }

    return object.height / 2.0f;
}

void applyFloorContact(Object3D& object)
{
    const float bottom =
        getObjectBottom(object);

    // No contact if the object is above the floor.
    if (bottom > 0.0f)
        return;

    const float penetration =
        -bottom;

    // Rigid-contact parameters.
    const float contactStiffness =
        5000.0f;

    const float contactDamping =
        180.0f;

    // Normal force:
    //
    // F_n = K_p delta - K_d v_n
    //
    // where delta is the penetration depth.
    float normalForce =
        contactStiffness * penetration
        - contactDamping * object.vz;

    // The floor can push the object upward, but cannot pull it.
    if (normalForce < 0.0f)
        normalForce = 0.0f;

    object.fz += normalForce;

    // Horizontal floor friction is handled in resolveFloorContact()
    // using stable velocity damping, avoiding numerical chatter.
}

// ============================================================
// RIGID FLOOR CONSTRAINT
// ============================================================
// This function is executed AFTER numerical integration.
// Therefore, even if Euler integration produces a tiny
// penetration during a time step, the object is immediately
// projected back onto the floor.
// ============================================================

void resolveFloorContact(Object3D& object)
{
    const float floorHeight =
        getObjectFloorHeight(object);

    // Resolve a collision only when the object reaches/crosses
    // the floor. Save the impact speed before correcting position.
    if (object.z <= floorHeight)
    {
        const float impactSpeed =
            (object.vz < 0.0f) ? -object.vz : 0.0f;

        object.z = floorHeight;

        // Bounce after a real impact. Low-speed contacts are
        // treated as resting contact to avoid endless micro-bounces.
        const float bounceThreshold = 0.70f;
        if (impactSpeed > bounceThreshold)
        {
            object.vz = object.restitution * impactSpeed;
        }
        else
        {
            object.vz = 0.0f;
        }

        // Friction/damping on the floor. Exponential damping is
        // stable and avoids the velocity sign-flipping caused by
        // applying a constant friction force near zero speed.
        const float horizontalDamping = 7.0f;
        const float dampingFactor =
            std::exp(-horizontalDamping * PHYSICS_DT);

        object.vx *= dampingFactor;
        object.vy *= dampingFactor;

        // Static-friction dead zone: the object eventually comes
        // to rest without sliding or jittering indefinitely.
        if (std::fabs(object.vx) < 0.025f)
            object.vx = 0.0f;
        if (std::fabs(object.vy) < 0.025f)
            object.vy = 0.0f;
    }
}

// ============================================================
// GRIPPER TARGET
// ============================================================

void getGripperTarget(
    float& x,
    float& y,
    float& z
)
{
    calculateEndEffector(
        x,
        y,
        z
    );

    float dx;
    float dy;
    float dz;

    calculateEndEffectorDirection(
        dx,
        dy,
        dz
    );

    // Move slightly forward from the wrist
    // so that the object is located between
    // the two fingers.
    x += dx * 0.45f;
    y += dy * 0.45f;
    z += dz * 0.45f;
}

// ============================================================
// DISTANCE TO GRIPPER
// ============================================================

float distanceToGripper(
    const Object3D& object
)
{
    float gx;
    float gy;
    float gz;

    getGripperTarget(
        gx,
        gy,
        gz
    );

    return vectorMagnitude(
        object.x - gx,
        object.y - gy,
        object.z - gz
    );
}

// ============================================================
// OBJECT CAN BE GRABBED
// ============================================================

bool objectCanBeGrabbed(
    const Object3D& object
)
{
    float gx, gy, gz;
    getGripperTarget(gx, gy, gz);

    const float distance = vectorMagnitude(
        object.x - gx,
        object.y - gy,
        object.z - gz
    );

    // Opening is the offset of each finger from the centerline.
    // Keep a small clearance, but do not make the condition
    // impossible for the sphere/cylinder used in this scene.
    const float requiredOpening = object.radius + 0.10f;
    if (gripperOpening < requiredOpening)
        return false;

    // Capture is allowed only when the object centre is near the gripping
    // region between the fingers. The tolerance includes the object radius,
    // since the wrist/gripper reference point is not the object centre.
    // This avoids making a valid grasp impossible while still preventing
    // grabbing objects that are merely near the arm body.
    // A forgiving capture tolerance helps when the visual gripper and its
    // kinematic reference point are not perfectly coincident. The test is
    // still centred on the gripper target, not on the arm links/body.
    const float captureDistance = object.radius + 0.65f;
    return distance <= captureDistance;
}

// ============================================================
// TRY GRAB
// ============================================================

void tryGrab()
{
    if (grabbedObject != -1)
        return;

    const bool sphereReady = objectCanBeGrabbed(sphere);
    const bool cylinderReady = objectCanBeGrabbed(cylinder);

    if (!sphereReady && !cylinderReady)
        return;

    // If both objects happen to be in range, grab the nearer one.
    if (sphereReady && cylinderReady)
    {
        if (distanceToGripper(sphere) <= distanceToGripper(cylinder))
        {
            grabbedObject = 0;
            sphere.grabbed = true;
            sphere.vx = sphere.vy = sphere.vz = 0.0f;
        }
        else
        {
            grabbedObject = 1;
            cylinder.grabbed = true;
            cylinder.vx = cylinder.vy = cylinder.vz = 0.0f;
        }
        return;
    }

    if (sphereReady)
    {
        grabbedObject = 0;
        sphere.grabbed = true;
        sphere.vx = sphere.vy = sphere.vz = 0.0f;
    }
    else
    {
        grabbedObject = 1;
        cylinder.grabbed = true;
        cylinder.vx = cylinder.vy = cylinder.vz = 0.0f;
    }
}

// ============================================================
// GRIPPER SPRING-DAMPER FORCE
// ============================================================

void applyGripperForce(
    Object3D& object
)
{
    if (!object.grabbed)
        return;

    float targetX;
    float targetY;
    float targetZ;

    getGripperTarget(
        targetX,
        targetY,
        targetZ
    );

    // Position error
    float ex =
        targetX -
        object.x;

    float ey =
        targetY -
        object.y;

    float ez =
        targetZ -
        object.z;

    // Relative velocity
    float evx =
        -object.vx;

    float evy =
        -object.vy;

    float evz =
        -object.vz;

    // Spring-damper model:
    //
    // F = K e + C de/dt
    //
    float fx =
        gripStiffness * ex
        +
        gripDamping * evx;

    float fy =
        gripStiffness * ey
        +
        gripDamping * evy;

    float fz =
        gripStiffness * ez
        +
        gripDamping * evz;

    float magnitude =
        vectorMagnitude(
            fx,
            fy,
            fz
        );

    // Saturation of gripping force
    if (magnitude > maxGripForce)
    {
        float scale =
            maxGripForce /
            magnitude;

        fx *= scale;
        fy *= scale;
        fz *= scale;
    }

    object.fx += fx;
    object.fy += fy;
    object.fz += fz;

}

// ============================================================
// RESET FORCES
// ============================================================

void resetForces(
    Object3D& object
)
{
    object.fx = 0.0f;
    object.fy = 0.0f;
    object.fz = 0.0f;
}

// ============================================================
// NUMERICAL INTEGRATION
// ============================================================

void integratePhysics(
    Object3D& object,
    float dt
)
{
    // Newton's second law:
    //
    // a = F / m

    float ax =
        object.fx /
        object.mass;

    float ay =
        object.fy /
        object.mass;

    float az =
        object.fz /
        object.mass;

    // Euler integration for velocity
    object.vx +=
        ax * dt;

    object.vy +=
        ay * dt;

    object.vz +=
        az * dt;

    // Velocity saturation
    const float maxVelocity =
        12.0f;

    float velocityMagnitude =
        vectorMagnitude(
            object.vx,
            object.vy,
            object.vz
        );

    if (velocityMagnitude >
        maxVelocity)
    {
        float scale =
            maxVelocity /
            velocityMagnitude;

        object.vx *= scale;
        object.vy *= scale;
        object.vz *= scale;
    }

    // Euler integration for position
    object.x +=
        object.vx * dt;

    object.y +=
        object.vy * dt;

    object.z +=
        object.vz * dt;
}

// ============================================================
// UPDATE SINGLE OBJECT
// ============================================================

void updateObjectPhysics(
    Object3D& object
)
{
    // A grasped object is kinematically attached to the gripper.
    // Grasp clearance is provided by requiring gripperOpening >= object.radius + 0.10f;
    // this leaves a small visible gap between each finger and the object's sides.
    // This avoids the old spring-force approach, which could fail
    // to carry the object because gravity and force saturation kept
    // it near the floor. Its velocity is estimated from the target
    // motion so that releasing it preserves the motion naturally.
    if (object.grabbed)
    {
        float targetX, targetY, targetZ;
        getGripperTarget(targetX, targetY, targetZ);

        object.vx = (targetX - object.x) / PHYSICS_DT;
        object.vy = (targetY - object.y) / PHYSICS_DT;
        object.vz = (targetZ - object.z) / PHYSICS_DT;

        object.x = targetX;
        object.y = targetY;
        object.z = targetZ;
        resetForces(object);
        return;
    }

    resetForces(object);

    // Gravity
    applyGravity(object);

    // Floor contact
    applyFloorContact(object);

    // Gripper force
    if (object.grabbed)
    {
        applyGripperForce(object);
    }

    // Integrate dynamics
    integratePhysics(
        object,
        PHYSICS_DT
    );

    // Enforce the rigid floor constraint AFTER integration.
    // This is what prevents the sphere/cylinder from passing
    // through the floor due to the discrete time step.
    resolveFloorContact(object);
}

// ============================================================
// OBJECT-OBJECT COLLISION DETECTION AND RESPONSE
// ============================================================
// The objects are represented for collision purposes by vertical
// bounding volumes: a horizontal disk of radius `radius` and the
// object's displayed vertical extent. This is sufficient for the
// sphere/cylinder scene and prevents them from passing through one
// another or settling inside one another.

float objectHalfHeight(const Object3D& object)
{
    return (object.type == SPHERE_OBJECT)
        ? object.radius
        : 0.5f * object.height;
}

void resolveObjectCollision(Object3D& a, Object3D& b)
{
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float distanceXY = std::sqrt(dx * dx + dy * dy);
    const float overlapXY = a.radius + b.radius - distanceXY;

    const float halfA = objectHalfHeight(a);
    const float halfB = objectHalfHeight(b);
    const float bottomA = a.z - halfA;
    const float topA = a.z + halfA;
    const float bottomB = b.z - halfB;
    const float topB = b.z + halfB;
    const float overlapZ = std::min(topA, topB) - std::max(bottomA, bottomB);

    if (overlapXY <= 0.0f || overlapZ <= 0.0f)
        return;

    // If one object is held, keep it attached to the gripper and
    // move the free object out of the held object's collision volume.
    const bool aFixed = a.grabbed;
    const bool bFixed = b.grabbed;
    if (aFixed && bFixed)
        return;

    // Choose the axis with the least penetration. Vertical resolution
    // produces stable stacking; horizontal resolution separates objects
    // that collide while resting side-by-side on the floor.
    if (overlapZ < overlapXY)
    {
        const bool aIsLower = a.z <= b.z;
        Object3D& lower = aIsLower ? a : b;
        Object3D& upper = aIsLower ? b : a;
        const float penetration = std::min(topA, topB) - std::max(bottomA, bottomB);

        if (aFixed || bFixed)
        {
            if (aFixed) {
                if (a.z <= b.z) b.z += penetration; else b.z -= penetration;
            } else {
                if (b.z <= a.z) a.z += penetration; else a.z -= penetration;
            }
        }
        else
        {
            lower.z -= 0.5f * penetration;
            upper.z += 0.5f * penetration;
        }

        // Remove velocity directed into the contact and apply a modest
        // bounce only on a sufficiently energetic impact.
        Object3D& moving = (aFixed || bFixed) ? (aFixed ? b : a) : upper;
        if (moving.vz < 0.0f && moving.z >= lower.z)
            moving.vz = 0.0f;
        else if (moving.vz > 0.0f && moving.z <= lower.z)
            moving.vz = 0.0f;
    }
    else
    {
        float nx, ny;
        if (distanceXY > 1.0e-5f) {
            nx = dx / distanceXY;
            ny = dy / distanceXY;
        } else {
            nx = 1.0f; ny = 0.0f;
        }

        const float penetration = overlapXY;
        if (aFixed)
        {
            b.x += nx * penetration;
            b.y += ny * penetration;
        }
        else if (bFixed)
        {
            a.x -= nx * penetration;
            a.y -= ny * penetration;
        }
        else
        {
            a.x -= 0.5f * nx * penetration;
            a.y -= 0.5f * ny * penetration;
            b.x += 0.5f * nx * penetration;
            b.y += 0.5f * ny * penetration;
        }

        const float relativeNormalVelocity =
            (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
        if (relativeNormalVelocity < 0.0f)
        {
            const float invMassA = aFixed ? 0.0f : 1.0f / a.mass;
            const float invMassB = bFixed ? 0.0f : 1.0f / b.mass;
            const float invMassSum = invMassA + invMassB;
            if (invMassSum > 0.0f)
            {
                const float impulse = -(1.0f + 0.05f) * relativeNormalVelocity / invMassSum;
                a.vx -= impulse * invMassA * nx;
                a.vy -= impulse * invMassA * ny;
                b.vx += impulse * invMassB * nx;
                b.vy += impulse * invMassB * ny;
            }
        }
    }

    // Keep both objects above the floor after positional correction.
    if (!a.grabbed && a.z < getObjectFloorHeight(a)) {
        a.z = getObjectFloorHeight(a);
        if (a.vz < 0.0f) a.vz = 0.0f;
    }
    if (!b.grabbed && b.z < getObjectFloorHeight(b)) {
        b.z = getObjectFloorHeight(b);
        if (b.vz < 0.0f) b.vz = 0.0f;
    }
}

// ============================================================
// ARM-OBJECT COLLISION DETECTION
// ============================================================
// The upper arm and forearm are approximated as capsules. The
// object is pushed out of the capsule when overlap is detected.
// This is a practical collision proxy for this GLUT demonstration.

float pointSegmentDistance(float px, float py, float pz,
                           float ax, float ay, float az,
                           float bx, float by, float bz,
                           float& qx, float& qy, float& qz)
{
    const float abx = bx-ax, aby = by-ay, abz = bz-az;
    const float denom = abx*abx + aby*aby + abz*abz;
    float t = 0.0f;
    if (denom > 1.0e-8f)
        t = clampValue(((px-ax)*abx + (py-ay)*aby + (pz-az)*abz)/denom, 0.0f, 1.0f);
    qx = ax + t*abx; qy = ay + t*aby; qz = az + t*abz;
    return vectorMagnitude(px-qx, py-qy, pz-qz);
}

void resolveArmObjectCollision(Object3D& object)
{
    if (object.grabbed) return; // the gripper intentionally carries this object

    const float a = baseAngle * (float)M_PI / 180.0f;
    const float t1 = shoulderAngle * (float)M_PI / 180.0f;
    const float t2 = (shoulderAngle + elbowAngle) * (float)M_PI / 180.0f;
    const float sx = 0.0f, sy = 0.0f, sz = baseHeight + 0.35f;
    const float ex = L1*std::cos(t1)*std::cos(a);
    const float ey = L1*std::cos(t1)*std::sin(a);
    const float ez = sz + L1*std::sin(t1);
    const float wx = ex + L2*std::cos(t2)*std::cos(a);
    const float wy = ey + L2*std::cos(t2)*std::sin(a);
    const float wz = ez + L2*std::sin(t2);
    const float armRadius = 0.47f; // half-width of the thicker arm, with small tolerance
    const float objectRadius = object.radius + (object.type == CYLINDER_OBJECT ? 0.08f : 0.0f);

    for (int link=0; link<2; ++link)
    {
        const float ax = (link==0 ? sx : ex), ay = (link==0 ? sy : ey), az = (link==0 ? sz : ez);
        const float bx = (link==0 ? ex : wx), by = (link==0 ? ey : wy), bz = (link==0 ? ez : wz);
        float qx, qy, qz;
        const float dist = pointSegmentDistance(object.x, object.y, object.z, ax, ay, az, bx, by, bz, qx, qy, qz);
        const float minDist = armRadius + objectRadius;
        if (dist >= minDist) continue;

        float nx = object.x-qx, ny = object.y-qy, nz = object.z-qz;
        float nmag = vectorMagnitude(nx,ny,nz);
        if (nmag < 1.0e-5f) { nx=0.0f; ny=0.0f; nz=1.0f; nmag=1.0f; }
        nx/=nmag; ny/=nmag; nz/=nmag;
        const float penetration = minDist-dist;
        object.x += nx*(penetration+0.002f);
        object.y += ny*(penetration+0.002f);
        object.z += nz*(penetration+0.002f);
        const float vn = object.vx*nx + object.vy*ny + object.vz*nz;
        if (vn < 0.0f) { object.vx -= vn*nx; object.vy -= vn*ny; object.vz -= vn*nz; }
        if (object.z < getObjectFloorHeight(object)) {
            object.z = getObjectFloorHeight(object);
            if (object.vz < 0.0f) object.vz = 0.0f;
        }
    }
}

// ============================================================
// PHYSICS UPDATE
// ============================================================

void updatePhysics()
{
    updateObjectPhysics(sphere);
    updateObjectPhysics(cylinder);

    // Multiple short solver passes reduce residual overlap and improve
    // stability when an object is released while touching another.
    for (int pass = 0; pass < 4; ++pass)
    {
        resolveObjectCollision(sphere, cylinder);
        resolveArmObjectCollision(sphere);
        resolveArmObjectCollision(cylinder);
    }
}

// ============================================================
// RELEASE
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
// FORCE ARROW
// ============================================================

void drawArrow(
    float x,
    float y,
    float z,
    float fx,
    float fy,
    float fz,
    float scale
)
{
    float magnitude =
        vectorMagnitude(
            fx,
            fy,
            fz
        );

    if (magnitude < 0.01f)
        return;

    float dx =
        fx / magnitude;

    float dy =
        fy / magnitude;

    float dz =
        fz / magnitude;

    float length =
        std::min(
            magnitude * scale,
            2.0f
        );

    float endX =
        x + dx * length;

    float endY =
        y + dy * length;

    float endZ =
        z + dz * length;

    glDisable(GL_LIGHTING);

    glLineWidth(3.0f);

    glColor3f(
        1.0f,
        0.2f,
        0.1f
    );

    glBegin(GL_LINES);

    glVertex3f(
        x,
        y,
        z
    );

    glVertex3f(
        endX,
        endY,
        endZ
    );

    glEnd();

    glPushMatrix();

    glTranslatef(
        endX,
        endY,
        endZ
    );

    glutSolidCone(
        0.08f,
        0.25f,
        8,
        4
    );

    glPopMatrix();

    glEnable(GL_LIGHTING);
}

// ============================================================
// FORCE VISUALIZATION
// ============================================================

void drawForces()
{
    if (!showForces)
        return;

    drawArrow(
        sphere.x,
        sphere.y,
        sphere.z,
        sphere.fx,
        sphere.fy,
        sphere.fz,
        0.03f
    );

    drawArrow(
        cylinder.x,
        cylinder.y,
        cylinder.z,
        cylinder.fx,
        cylinder.fy,
        cylinder.fz,
        0.03f
    );
}

// ============================================================
// ROBOT
// ============================================================

void drawRobot()
{
    glPushMatrix();

    // Base rotation
    glRotatef(
        baseAngle,
        0.0f,
        0.0f,
        1.0f
    );

    // Base
    drawRobotBase();

    // Move to shoulder
    glTranslatef(
        0.0f,
        0.0f,
        baseHeight + 0.35f
    );

    // Shoulder joint
    drawJoint(
        0.40f
    );

    // Shoulder rotation
    glRotatef(
        -shoulderAngle,
        0.0f,
        1.0f,
        0.0f
    );

    // Upper arm
    drawUpperArm();

    // Move to elbow
    glTranslatef(
        L1,
        0.0f,
        0.0f
    );

    // Elbow joint
    drawJoint(
        0.32f
    );

    // Elbow rotation
    glRotatef(
        -elbowAngle,
        0.0f,
        1.0f,
        0.0f
    );

    // Forearm
    drawForearm();

    // Move exactly to the end of forearm
    glTranslatef(
        L2,
        0.0f,
        0.0f
    );

    // Gripper
    drawGripper();

    glPopMatrix();
}

// ============================================================
// LIGHTING
// ============================================================

void setupLighting()
{
    glEnable(
        GL_LIGHTING
    );

    glEnable(
        GL_LIGHT0
    );

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

    glEnable(
        GL_COLOR_MATERIAL
    );

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
        cameraYaw *
        M_PI /
        180.0f;

    float pitchRad =
        cameraPitch *
        M_PI /
        180.0f;

    float camX =
        cameraDistance *
        std::cos(pitchRad) *
        std::sin(yawRad);

    float camY =
        cameraDistance *
        std::cos(pitchRad) *
        std::cos(yawRad);

    float camZ =
        cameraDistance *
        std::sin(pitchRad);

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

    drawFloor();

    drawGrid();

    drawObjects();

    drawRobot();

    drawForces();

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
        (double)width /
        (double)height,
        0.1,
        100.0
    );

    glMatrixMode(
        GL_MODELVIEW
    );
}

// ============================================================
// RESET SIMULATION
// ============================================================

void resetSimulation()
{
    // Robot
    baseAngle = 0.0f;

    shoulderAngle = 55.0f;

    elbowAngle = -15.0f;

    gripperOpening = 0.88f;

    // Grabbing state
    grabbedObject = -1;

    sphere.grabbed = false;
    cylinder.grabbed = false;

    // Sphere initial condition
    sphere.x = 3.5f;
    sphere.y = 0.0f;
    sphere.z = sphere.radius;

    sphere.vx = 0.0f;
    sphere.vy = 0.0f;
    sphere.vz = 0.0f;

    // Cylinder initial condition
    cylinder.x = -3.0f;
    cylinder.y = 1.5f;

    cylinder.z =
        cylinder.height / 2.0f;

    cylinder.vx = 0.0f;
    cylinder.vy = 0.0f;
    cylinder.vz = 0.0f;

    resetForces(sphere);

    resetForces(cylinder);
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

            shoulderAngle =
                clampValue(
                    shoulderAngle,
                    0.0f,
                    80.0f
                );
            // Keep both links on or above the grid plane.
            if (shoulderAngle + elbowAngle < 0.0f)
                elbowAngle = -shoulderAngle;
            if (shoulderAngle + elbowAngle > 90.0f)
                elbowAngle = 90.0f - shoulderAngle;

            break;

        case 's':
        case 'S':

            shoulderAngle -= 3.0f;

            shoulderAngle =
                clampValue(
                    shoulderAngle,
                    0.0f,
                    80.0f
                );
            // Keep both links on or above the grid plane.
            if (shoulderAngle + elbowAngle < 0.0f)
                elbowAngle = -shoulderAngle;
            if (shoulderAngle + elbowAngle > 90.0f)
                elbowAngle = 90.0f - shoulderAngle;

            break;

        // ----------------------------------------------------
        // ELBOW
        // ----------------------------------------------------

        case 'q':
        case 'Q':

            elbowAngle += 3.0f;

            elbowAngle =
                clampValue(
                    elbowAngle,
                    -shoulderAngle,
                    90.0f - shoulderAngle
                );

            break;

        case 'e':
        case 'E':

            elbowAngle -= 3.0f;

            elbowAngle =
                clampValue(
                    elbowAngle,
                    -shoulderAngle,
                    90.0f - shoulderAngle
                );

            break;

        // ----------------------------------------------------
        // OPEN GRIPPER
        // ----------------------------------------------------

        case 'z':
        case 'Z':

            gripperOpening += 0.04f;

            if (gripperOpening >
                maximumOpening)
            {
                gripperOpening =
                    maximumOpening;
            }

            break;

        // ----------------------------------------------------
        // CLOSE GRIPPER
        // ----------------------------------------------------

        case 'x':
        case 'X':

            gripperOpening -= 0.04f;

            if (gripperOpening <
                minimumOpening)
            {
                gripperOpening =
                    minimumOpening;
            }

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

            resetSimulation();

            break;

        // ----------------------------------------------------
        // FORCE VISUALIZATION
        // ----------------------------------------------------

        case 'f':
        case 'F':

            showForces =
                !showForces;

            break;

        // ----------------------------------------------------
        // ZOOM IN
        // ----------------------------------------------------

        case '+':
        case '=':

            cameraDistance -= 0.5f;

            if (cameraDistance < 5.0f)
                cameraDistance = 5.0f;

            break;

        // ----------------------------------------------------
        // ZOOM OUT
        // ----------------------------------------------------

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
// TIMER
// ============================================================

void timer(
    int value
)
{
    updatePhysics();

    glutPostRedisplay();

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

    glShadeModel(
        GL_SMOOTH
    );

    setupLighting();

    glEnable(
        GL_COLOR_MATERIAL
    );
}

// ============================================================
// MAIN
// ============================================================

int main(
    int argc,
    char** argv
)
{
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
        "OpenGL Robotic Arm - Physics Simulation"
    );

    initOpenGL();

    // Callbacks
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

    // Initial state
    resetSimulation();

    // Main GLUT loop
    glutMainLoop();

    return 0;
}
