// Original u430: integration, collision callbacks and all projectile visual lifetimes.
#include "Ballistics.h"
#include "App.h"
#include "DGHelper.h"
#include "CSwerveGame.h"
#include "CUnitBody.h"
#include "CUnit.h"
#include "CUnitsController.h"
#include "CGrenade.h"
#include "CGrenadeManager.h"
#include "CDH_SoundManager.h"
#include "CDH_Animation.h"
#include "CDH_Weapon.h"
#include "CSceneLighting.h"
#include "DGWorld.h"
#include "dgCamera.h"
#include "CRandGen.h"
#include "CSwerve.h"
#include "Vector_ss.h"
#include "CDH_BasicGameData.h"
#include <string.h>

using com::glu::platform::core::CRandGen;
typedef swerve::Group<CSwerveAssertingErrorHandler> Group;
typedef swerve::World<CSwerveAssertingErrorHandler> World;
typedef swerve::Node<CSwerveAssertingErrorHandler> Node;
typedef swerve::Transform<CSwerveAssertingErrorHandler> Transform;

Ballistics::Projectile* Ballistics::g_CurrentlyCollidingProjectile;
Ballistics* Ballistics::g_CurrentlySimulatedBallistics;
Fragments* Fragments::g_CurrentlySimulatedFragments;

namespace {
// These helpers retain the game's out-of-line inverse square root, including zero input.
inline float Length(const vec3& value)
{
    float square = value.LengthSqr();
    return MathLib::InvSqrt(square) * square;
}
inline vec3 Unit(const vec3& value)
{
    return value * MathLib::InvSqrt(value.LengthSqr());
}
inline CSwerveGame* Game()
{
    return ((App*)WindowApp::m_instance)->m_helper->m_swerveGame;
}
}

Ballistics::Ballistics(int capacity, float maxAge, vec3 gravity, CCollision* collision)
    : m_gravity()
{
    m_collision = collision;
    m_capacity = capacity;
    m_next = 0;
    m_activeCount = 0;
    m_activeIndices = new int[capacity];
    m_projectiles = new Projectile[m_capacity];
    m_maxAge = maxAge;
    m_gravity = gravity;
    for (int index=0; index<m_capacity; index++) {
        m_projectiles[index].m_active = false;
    }
    m_normalBounce = 0.5f;
    m_tangentRetention = 0.5f;
    m_drag = 0.0f;
    m_flags = 1;
}
Ballistics::~Ballistics()
{
    delete[] m_activeIndices;
    delete[] m_projectiles;
}
void Ballistics::Simulate(float elapsed)
{
    g_CurrentlySimulatedBallistics = this;
    OnPreSimulate(elapsed);
    m_activeCount = 0;
    for (int index=0; index<m_capacity; index++) {
        if (m_projectiles[index].m_active) {
            m_activeIndices[m_activeCount] = index;
            m_activeCount++;
            m_projectiles[index].m_previous = m_projectiles[index].m_position;
        }
    }
    float remaining = elapsed;
    float step = elapsed;
    while (remaining > 0.0f) {
        step = 1.0f / 30.0f;
        if (remaining < step) {
            step = remaining;
        }
        for (int index=0; index<m_activeCount; index++) {
            Projectile* projectile = &m_projectiles[m_activeIndices[index]];
            if (projectile->m_active) {
                projectile->m_age += step;
                if (!(projectile->m_age > m_maxAge)) {
                    projectile->m_position = projectile->m_position + projectile->m_velocity * step;
                    projectile->m_velocity = projectile->m_velocity + m_gravity * step;
                    if (m_drag > 0.0f) {
                        projectile->m_velocity = projectile->m_velocity - projectile->m_velocity * m_drag * step;
                    }
                } else {
                    projectile->m_active = false;
                    OnActivate(projectile);
                }
            }
        }
        remaining -= step;
    }
    if (m_collision != NULL && m_activeCount != 0) {
        vec3 collisionNormal;
        vec3 collisionPoint;
        for (int index=0; index<m_activeCount; index++) {
            Projectile* projectile = &m_projectiles[m_activeIndices[index]];
            g_CurrentlyCollidingProjectile = projectile;
            if (projectile->m_active == false) {
                continue;
            }
            bool kinematic = false;
            if ((m_flags & 2) != 0) {
                kinematic = projectile->m_kinematic;
            }
            if (!(Length(projectile->m_position-projectile->m_previous) > 0.0f)) {
                continue;
            }
            vec3 hit;
            vec3 normal;
            CCollision::Triangle triangle;
            CCollision::Line line;
            line.m_from = projectile->m_previous;
            line.m_to = projectile->m_position;
            bool collided = m_collision->IntersectNearest(line, hit, normal, triangle, kinematic);
            float fraction = Length(hit-projectile->m_previous) / Length(projectile->m_previous-projectile->m_position);
            if (fraction > 1.0f) {
                collided = false;
            }
            if (kinematic && (m_flags & 4) != 0) {
                if (m_collision->m_lastKinematic != NULL && m_collision->m_lastKinematic->m_body != NULL) {
                    collided = m_collision->m_lastKinematic->m_body->Pick(line, hit, normal);
                }
                if (collided == false) {
                    collided = m_collision->IntersectNearest(line, hit, normal, triangle, false);
                    fraction = Length(hit-projectile->m_previous) / Length(projectile->m_previous-projectile->m_position);
                    if (fraction > 1.0f) {
                        continue;
                    }
                }
            }
            if (collided == false) {
                continue;
            }
            collisionPoint = projectile->m_previous + (projectile->m_position-projectile->m_previous)*fraction;
            collisionNormal = normal;
            int bounce = OnCollision(projectile, &collisionPoint, &collisionNormal, &triangle);
            if (projectile->m_active == false) {
                OnActivate(projectile);
            }
            if (bounce == 1 && projectile->m_active) {
                vec3 normalVelocity = collisionNormal * projectile->m_velocity.Dot(collisionNormal);
                vec3 tangentVelocity = projectile->m_velocity - normalVelocity;
                projectile->m_velocity = tangentVelocity*m_tangentRetention - normalVelocity*m_normalBounce;
                projectile->m_position = collisionPoint;
            }
        }
        g_CurrentlyCollidingProjectile = NULL;
    }
    // The original forwards the final integration step, including a short final remainder.
    OnPostSimulate(step);
    g_CurrentlySimulatedBallistics = NULL;
}
int Ballistics::AddProjectile(const vec3& position, const vec3& velocity, float spread, void* userData)
{
    int index = m_next;
    Projectile* projectile = &m_projectiles[index];
    m_next++;
    if (m_next == m_capacity) {
        m_next = 0;
    }
    if (projectile->m_active) {
        projectile->m_active = false;
        OnActivate(projectile);
    }
    projectile->m_active = true;
    projectile->m_age = 0.0f;
    projectile->m_position = position;
    projectile->m_previous = position;
    projectile->m_velocity = velocity;
    if (spread != 0.0f) {
        vec3 forward = Unit(projectile->m_velocity);
        vec3 right = Unit(vec3(forward.m_y,forward.m_z,forward.m_x).Cross(forward));
        vec3 up = Unit(forward.Cross(right));
        Angles angles(forward, up);
        Mat4x4 matrix(1.0f);
        matrix.SetAngles(angles);
        Quaternion orientation;
        memset(&orientation,0,sizeof(orientation));
        matrix.ToQuaternion(orientation);
        float first = (float)(10000-CRandGen::GetInstance()->GetRand(20000));
        float second = (float)(10000-CRandGen::GetInstance()->GetRand(20000));
        Quaternion horizontal;
        horizontal.m_x=right.m_x; horizontal.m_y=right.m_y; horizontal.m_z=right.m_z;
        horizontal.m_w=(first*0.0001f*spread*3.1415925f)/360.0f;
        Quaternion vertical;
        vertical.m_x=up.m_x; vertical.m_y=up.m_y; vertical.m_z=up.m_z;
        vertical.m_w=(second*0.0001f*spread*3.1415925f)/360.0f;
        orientation = orientation.Mul(horizontal.Mul(vertical));
        orientation.ToAngles_ByKashtur(angles);
        vec3 direction = Unit(angles.Forward());
        projectile->m_velocity = direction*Length(projectile->m_velocity);
    }
    projectile->m_kinematic = true;
    projectile->m_userData = userData;
    OnActivate(projectile);
    return index;
}
void Ballistics::SetGravity(const vec3& gravity) { m_gravity=gravity; }
void Ballistics::SetBouncing(float normal,float tangent) { m_normalBounce=normal; m_tangentRetention=tangent; }
void Ballistics::SetDrag(float drag) { m_drag=drag; }
void Ballistics::SetCollisionFlags(unsigned int flags) { m_flags=flags; }

Fragments::Fragments(int capacity,float age,vec3 gravity,CCollision* collision,EParticleType type,vec3 size,const char* model)
    : Ballistics(capacity,age,gravity,collision), m_type(type)
{
    // Original stores m_size after the member default constructors (memset 0x48).
    m_size=size;
    m_particles=new Particle[capacity];
    m_particleNext=0;
    m_variantNext=0;
    m_scene=World(App::LoadObject3D(model));
    m_root=Group(m_scene.find(DGHelper::getSwerveID("RootNode")));
    for (int index=0;index<16;index++) {
        XString name=XString::Format(L"Particle%02dTransform",index+1);
        m_variants[index]=Group(m_root.find(DGHelper::getSwerveID(XString::AnsiString(name.c_str()))));
    }
}
Fragments::~Fragments() { delete[] m_particles; }
void Fragments::OnPreSimulate(float) { g_CurrentlySimulatedFragments=this; }
void Fragments::OnPostSimulate(float) { g_CurrentlySimulatedFragments=NULL; }
int Fragments::OnCollision(Projectile* projectile,vec3*,vec3*,CCollision::Triangle* triangle)
{
    if (triangle->m_meshIndex == -1) {
        if (((Particle*)projectile->m_userData)->m_removeOnKinematic != 0) {
            projectile->m_active=false;
            OnActivate(projectile);
            return 1;
        }
        return 0;
    }
    return 1;
}
int Fragments::AddProjectile(const vec3& position,const vec3& velocity,float spread,int removeOnKinematic)
{
    m_particles[m_particleNext].m_origin=position;
    m_particles[m_particleNext].m_variant=m_variantNext;
    m_particles[m_particleNext].m_rotation=(float)CRandGen::GetInstance()->GetRand(360);
    m_particles[m_particleNext].m_removeOnKinematic=removeOnKinematic;
    m_variantNext++;
    if (m_variantNext==16) { m_variantNext=0; }
    int projectile=Ballistics::AddProjectile(position,velocity,spread,&m_particles[m_particleNext]);
    m_particleNext++;
    if (m_particleNext>=m_capacity) { m_particleNext=0; }
    return projectile;
}
SmokeTrail::SmokeTrail(int capacity,float age,CCollision* collision)
    : Ballistics(capacity,age,vec3(),collision)
{
    m_scene=World(App::LoadObject3D("BIN_M3G_EFFECT_SMOKE_SYSTEM"));
    m_root=Group(m_scene.find(DGHelper::getSwerveID("RootNode")));
    for (int index=0;index<16;index++) {
        XString name=XString::Format(L"Particle%02dTransform",index+1);
        m_variants[index]=Group(m_root.find(DGHelper::getSwerveID(XString::AnsiString(name.c_str()))));
    }
    m_rotationNext=0;
    m_rotations=new Rotation[capacity];
    m_basis=Transform(CSwerve::GetInstance()->m_swerve);
    m_scratch=Transform(CSwerve::GetInstance()->m_swerve);
}
SmokeTrail::~SmokeTrail() { delete[] m_rotations; }
int SmokeTrail::AddProjectile(const vec3& position,const vec3& velocity,float spread)
{
    m_rotations[m_rotationNext].m_angle=(float)CRandGen::GetInstance()->GetRand(720)-360.0f;
    m_rotations[m_rotationNext].m_speed=(float)CRandGen::GetInstance()->GetRand(180)-90.0f;
    // Both original wraps store the selected angle unconditionally (as in Simulate).
    Rotation& firstRotation = m_rotations[m_rotationNext];
    firstRotation.m_angle = firstRotation.m_angle < -180.0f ? firstRotation.m_angle + 360.0f : firstRotation.m_angle;
    Rotation& secondRotation = m_rotations[m_rotationNext];
    secondRotation.m_angle = secondRotation.m_angle > 180.0f ? 360.0f - secondRotation.m_angle : secondRotation.m_angle;
    int projectileIndex = Ballistics::AddProjectile(position,velocity,spread,&m_rotations[m_rotationNext]);
    m_rotationNext++;
    if (m_rotationNext==m_capacity) { m_rotationNext=0; }
    // The base projectile index remains in r0 in the original tail.
    return projectileIndex;
}
void SmokeTrail::Simulate(float elapsed)
{
    Ballistics::Simulate(elapsed);
    int count=m_activeCount;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active) {
            Rotation* rotation=(Rotation*)projectile.m_userData;
            rotation->m_angle+=rotation->m_speed*elapsed;
            // ?: on purpose: the original stores the angle unconditionally after each wrap (select form).
            rotation->m_angle = (rotation->m_angle < -180.0f) ? rotation->m_angle+360.0f : rotation->m_angle;
            rotation->m_angle = (rotation->m_angle > 180.0f) ? 360.0f-rotation->m_angle : rotation->m_angle;
        }
    }
}
void Fragments::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    int count=m_activeCount;
    vec3 camera=Game()->GetCurrentCameraPos();
    Game()->m_camera->GetViewDirection();
    Game()->GetCurrentCameraRight();
    vec3 cameraUp=Game()->GetCurrentCameraUp();
    float nearScale=3.0f;
    float farScale=0.75f;
    if (m_type==PARTICLE_SEGMENT || m_type==PARTICLE_TRAIL) { nearScale=4.0f; farScale=0.5f; }
    Mat4x4 matrix(1.0f);
    Transform renderTransform(CSwerve::GetInstance()->m_swerve);
    Transform basis(CSwerve::GetInstance()->m_swerve);
    Transform scratch(CSwerve::GetInstance()->m_swerve);
    renderTransform.setIdentity();
    for (int index=0;index<16;index++) { m_variants[index].setRenderingEnable(false); }
    int batch=0;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active==false) { continue; }
        Particle* particle=(Particle*)projectile.m_userData;
        vec3 segment;
        vec3 center;
        if (m_type==PARTICLE_SEGMENT) {
            segment=projectile.m_position-projectile.m_previous;
            center=projectile.m_previous+segment*0.5f;
        } else if (m_type==PARTICLE_RAY) {
            segment=projectile.m_position-particle->m_origin;
            center=projectile.m_position-segment*0.5f;
        } else if (m_type==PARTICLE_TRAIL) {
            segment=projectile.m_position-particle->m_origin;
            center=projectile.m_previous+segment*0.5f;
        } else {
            segment=cameraUp;
            center=projectile.m_position;
        }
        vec3 view=center-camera;
        float fade=(100.0f-Length(view))/100.0f;
        if (fade<0.0f) { fade=0.0f; }
        if (fade>1.0f) { fade=1.0f; }
        vec3 front=Unit(view);
        vec3 axis=Unit(segment);
        vec3 side=Unit(axis.Cross(front));
        float scale=farScale*fade+nearScale*(1.0f-fade);
        vec3 first=axis*m_size.m_x*Length(segment)*scale;
        vec3 second=side*m_size.m_z*scale;
        vec3 third=front*m_size.m_x*Length(segment)*scale;
        float* values=matrix.ToFloatPtr();
        values[0]=first.m_x; values[4]=first.m_y; values[8]=first.m_z;
        values[1]=second.m_x; values[5]=second.m_y; values[9]=second.m_z;
        values[2]=third.m_x; values[6]=third.m_y; values[10]=third.m_z;
        if (m_type==PARTICLE_FLASH) {
            matrix.SetTranslation(vec3());
            basis.set(values);
            scratch.setIdentity();
            float rotation[4]={particle->m_rotation,third.m_x,third.m_y,third.m_z};
            scratch.postRotate(rotation);
            scratch.postMultiply(basis);
            basis.multiply(scratch);
            scratch.setIdentity();
            scratch.postTranslate(&center.m_x);
            scratch.postMultiply(basis);
        } else {
            matrix.SetTranslation(center);
            scratch.set(values);
        }
        m_variants[particle->m_variant].setTransform(scratch);
        m_variants[particle->m_variant].setRenderingEnable(true);
        m_variants[particle->m_variant].setAlphaFactor(1.0f-projectile.m_age/m_maxAge);
        batch++;
        if (batch==16) {
            graphics.render(m_root.m_interface,renderTransform.m_interface);
            for (int hidden=0;hidden<16;hidden++) { m_variants[hidden].setRenderingEnable(false); }
            batch=0;
        }
    }
    if (batch>0) { graphics.render(m_root.m_interface,renderTransform.m_interface); }
}
void SmokeTrail::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    int count=m_activeCount;
    vec3 camera=Game()->GetCurrentCameraPos();
    vec3 viewDirection=Game()->m_camera->GetViewDirection();
    Mat4x4 matrix(1.0f);
    Transform renderTransform(CSwerve::GetInstance()->m_swerve);
    renderTransform.setIdentity();
    int batch=0;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active==false) { continue; }
        float age=projectile.m_age/m_maxAge;
        if (age>1.0f) { age=1.0f; }
        float scale=MathLib::Pow(age,0.5f);
        if (scale<0.01f) { scale=0.01f; }
        vec3 center=projectile.m_previous+(projectile.m_position-projectile.m_previous)*0.5f;
        vec3 front=Unit(center-camera);
        vec3 axis=Unit(vec3(0,0,1));
        vec3 side=Unit(axis.Cross(front));
        vec3 first=axis*scale*75.0f;
        vec3 second=side*scale*75.0f;
        vec3 third=front*scale*75.0f;
        float* values=matrix.ToFloatPtr();
        values[0]=first.m_x; values[4]=first.m_y; values[8]=first.m_z;
        values[1]=second.m_x; values[5]=second.m_y; values[9]=second.m_z;
        values[2]=third.m_x; values[6]=third.m_y; values[10]=third.m_z;
        matrix.SetTranslation(vec3());
        m_basis.set(values);
        m_scratch.setIdentity();
        float rotation[4]={((Rotation*)projectile.m_userData)->m_angle,third.m_x,third.m_y,third.m_z};
        m_scratch.postRotate(rotation);
        m_scratch.postMultiply(m_basis);
        m_basis.multiply(m_scratch);
        m_scratch.setIdentity();
        m_scratch.postTranslate(&center.m_x);
        m_scratch.postMultiply(m_basis);
        m_variants[batch].setTransform(m_scratch);
        m_variants[batch].setAlphaFactor(1.0f-projectile.m_age/m_maxAge);
        batch++;
        if (batch==16) { graphics.render(m_root.m_interface,renderTransform.m_interface); batch=0; }
    }
    if (batch>0) {
        matrix.SetTranslation(camera-viewDirection*10000.0f);
        m_basis.set(matrix.ToFloatPtr());
        for (int index=batch;index<16;index++) { m_variants[index].setTransform(m_basis); }
        graphics.render(m_root.m_interface,renderTransform.m_interface);
    }
}
UnitBodyBallistics::UnitBodyBallistics(CCollision* collision,CAuxRender* render)
    : Ballistics(32,10.0f,vec3(0,0,-9.8f),collision),m_auxRender(render)
{
    m_normalBounce=0.25f; m_tangentRetention=0.01f; m_drag=0.5f;
}
UnitBodyBallistics::~UnitBodyBallistics() {}
int UnitBodyBallistics::OnCollision(Projectile* projectile,vec3*,vec3* normal,CCollision::Triangle*)
{
    projectile->m_active=MathLib::Fabs(normal->Dot(vec3(0,0,1)))<0.707f;
    projectile->m_velocity=vec3();
    return 1;
}
void UnitBodyBallistics::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>&) {} // Original return-only at 0xeea14.
SwerveBallistics::SwerveBallistics(int capacity,float age,CCollision* collision)
    : Ballistics(capacity,age,vec3(0,0,-9.8f),collision)
{
    m_normalBounce=0.2f; m_tangentRetention=0.5f; m_drag=0.1f;
}
SwerveBallistics::~SwerveBallistics()
{
    for (int index=0;index<m_capacity;index++) {
        if (m_projectiles[index].m_active && m_projectiles[index].m_userData!=NULL) {
            delete (Group*)m_projectiles[index].m_userData;
        }
    }
}
void SwerveBallistics::AddProjectile(Group group,const vec3& position,const vec3& velocity,float spread)
{
    Group* owned=new Group(group);
    Ballistics::AddProjectile(position,velocity,spread,owned);
}
void SwerveBallistics::PostSimulate()
{
    int count=m_activeCount;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active) {
            // Original reads m_userData before computing the movement.
            Group* group=(Group*)projectile.m_userData;
            vec3 movement=projectile.m_position-projectile.m_previous;
            group->translate(&movement.m_x);
        }
    }
}
void SwerveBallistics::OnActivate(Projectile* projectile)
{
    if (projectile->m_active==false) {
        Group* group=(Group*)projectile->m_userData;
        Group parent(group->getParent());
        parent.removeChild(*group);
        delete group;
        projectile->m_userData=NULL;
    }
}
int SwerveBallistics::OnCollision(Projectile* projectile,vec3*,vec3*,CCollision::Triangle*)
{
    if (projectile->m_velocity.Dot(vec3(0,0,1))>0.0f && Length(projectile->m_velocity)<=0.25f) {
        projectile->m_velocity=vec3();
    }
    return 1;
}
HitPoints::HitPoints(int capacity,float age,CCollision* collision)
    : Ballistics(capacity,age,vec3(0,0,2),collision),m_selected(-1)
{
}
HitPoints::~HitPoints()
{
    for (int index=0;index<m_capacity;index++) {
        if (m_projectiles[index].m_active && m_projectiles[index].m_userData!=NULL) {
            delete (Window**)m_projectiles[index].m_userData;
        }
    }
}
void HitPoints::AddWindow(Window* window)
{
    m_velocity=m_velocity*0.5f;
    AddWindow(window,m_position,m_velocity);
}
void HitPoints::AddWindow(Window* window,const vec3& position,const vec3& velocity)
{
    m_position=position; m_velocity=velocity;
    int selected=m_selected;
    if (selected>=0 && selected<m_activeCount) {
        // Original re-indexes after the byte store (no cached pointer).
        m_projectiles[m_activeIndices[selected]].m_active=false;
        OnActivate(&m_projectiles[m_activeIndices[selected]]);
    }
    Game()->AddToFront(window);
    Window** owned=new Window*(window);
    m_selected=Ballistics::AddProjectile(position,velocity,0.0f,owned);
}
void HitPoints::OnActivate(Projectile* projectile)
{
    if (projectile->m_active) { return; }
    if (m_selected>=0 && m_selected<m_activeCount && &m_projectiles[m_activeIndices[m_selected]]==projectile) {
        m_selected=-1;
    }
    Window** window=(Window**)projectile->m_userData;
    if (window!=NULL) {
        if (*window!=NULL) { (*window)->Close(); }
        delete window;
        projectile->m_userData=NULL;
    }
}
int HitPoints::OnCollision(Projectile*,vec3*,vec3*,CCollision::Triangle*) { return 1; }
void HitPoints::PostSimulate()
{
    DGCamera* camera=Game()->m_camera;
    vec3 up=Game()->GetCurrentCameraUp();
    Game()->GetCurrentCameraRight();
    int count=m_activeCount;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active) {
            Window* window=*(Window**)projectile.m_userData;
            int x,y;
            int width=window->m_width;
            int height=window->m_height;
            camera->GetScreenPosition(&x,&y,projectile.m_position);
            window->SetRect(x-width/2,y-height/2,width,height,true);
            window->SetOpacity(1.0f-projectile.m_age/m_maxAge);
        }
    }
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active==false) { continue; }
        vec3 force;
        Window* window=*(Window**)projectile.m_userData;
        for (int other=0;other<count;other++) {
            Projectile& second=m_projectiles[m_activeIndices[other]];
            if (other!=index && second.m_active) {
                Window* secondWindow=*(Window**)second.m_userData;
                int half=window->m_height/2;
                int secondHalf=secondWindow->m_height/2;
                float difference=(float)((window->m_y+half-secondWindow->m_y)-secondHalf);
                float combined=(float)(secondHalf+half);
                if (MathLib::Fabs(difference)<combined) {
                    float amount=(combined-MathLib::Fabs(difference))/combined;
                    if (amount>1.0f) { amount=1.0f; }
                    float separation=amount*10.0f+(1.0f-amount)*0.0f;
                    float sign=0.0f;
                    if (difference<0.0f) { sign=-1.0f; }
                    if (difference>0.0f) { sign=1.0f; }
                    force=force-up*separation*sign;
                }
            }
        }
        float elapsed=WindowApp::m_instance->m_frameTimeSeconds;
        projectile.m_velocity.m_x+=force.m_x*elapsed;
        projectile.m_velocity.m_y+=force.m_y*elapsed;
        float vertical=force.m_z*elapsed;
        if (vertical<0.0f) { vertical=0.0f; }
        projectile.m_velocity.m_z+=vertical;
    }
}

GrenadeLauncher::GrenadeLauncher(CCollision* collision,const char* model,const char* alternateModel)
    : Ballistics(32,10.0f,vec3(0,0,-9.8f),collision)
{
    m_model=World(App::LoadObject3D(model));
    m_alternateModel=World(App::LoadObject3D(alternateModel));
    m_smoke=new SmokeTrail(256,2.0f,NULL);
    m_flash=new Fragments(32,2.0f,vec3(),NULL,Fragments::PARTICLE_FLASH,vec3(33.3f,33.3f,33.3f),"BIN_M3G_EFFECT_FLASH_SYSTEM");
    m_flightNext=0;
    m_flights=new Flight[32];
    m_trailSpacing=0.5f; m_trailVelocity=0.5f; m_trailAge=15.0f;
    m_explosionRadius=5.0f;
    m_flags=7;
}
GrenadeLauncher::~GrenadeLauncher() { delete m_smoke; delete m_flash; delete[] m_flights; }
void GrenadeLauncher::SetTrail(float spacing,float velocity,float age)
{
    m_trailSpacing=spacing; m_trailVelocity=velocity; m_trailAge=age;
}
void GrenadeLauncher::SetExplosionRadius(float radius) { m_explosionRadius=radius; }
int GrenadeLauncher::OnCollision(Projectile* projectile,vec3* point,vec3* normal,CCollision::Triangle*)
{
    projectile->m_active=false;
    Flight* flight=(Flight*)projectile->m_userData;
    int count=m_flash->m_activeCount;
    for (int index=0;index<count;index++) {
        int active=m_flash->m_activeIndices[index];
        if (active==flight->m_flash1 || active==flight->m_flash2) {
            m_flash->m_projectiles[active].m_active=false;
            m_flash->OnActivate(&m_flash->m_projectiles[active]);
        }
    }
    App* app=(App*)WindowApp::m_instance;
    CDH_Weapon* weapon=app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
    Game()->BlastGrenade(*point,*normal,m_explosionRadius,weapon,NULL);
    *point=*point+*normal*0.1f;
    return 1;
}
int GrenadeLauncher::AddProjectile(const vec3& position,const vec3& velocity,float spread,vec3 aimDirection,bool alternate)
{
    m_flights[m_flightNext].m_trailDistance = 0.0f;
    m_flights[m_flightNext].m_distance = 0.0f;
    m_flights[m_flightNext].m_splitCount = 1;
    m_flights[m_flightNext].m_aimDirection=aimDirection;
    m_flights[m_flightNext].m_guided=Length(aimDirection)>0.0f;
    m_flights[m_flightNext].m_flash1=-1; m_flights[m_flightNext].m_flash2=-1; m_flights[m_flightNext].m_alternate=alternate;
    if (alternate==false && m_flights[m_flightNext].m_guided) {
        m_flights[m_flightNext].m_target=Game()->GetLaserSightRayCastResult(&m_flights[m_flightNext].m_targetValid);
    } else { m_flights[m_flightNext].m_targetValid=false; m_flights[m_flightNext].m_target=vec3(); }
    int projectileIndex = Ballistics::AddProjectile(position,velocity,spread,&m_flights[m_flightNext]);
    m_flightNext++;
    if (m_flightNext==m_capacity) { m_flightNext=0; }
    // The original preserves the base projectile index in r0 while advancing the flight slot.
    return projectileIndex;
}
void GrenadeLauncher::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    int count=m_activeCount;
    Transform transform(CSwerve::GetInstance()->m_swerve);
    while (count>0) {
        count--;
        Projectile& projectile=m_projectiles[m_activeIndices[count]];
        if (projectile.m_active) {
            Flight* flight=(Flight*)projectile.m_userData;
            {
                Node reference=Game()->GetRenderReferenceNode();
                reference.getTransformTo(Game()->m_cameraNode,transform);
            }
            Mat4x4 matrix;
            transform.get(matrix.ToFloatPtr());
            matrix.SetTranslation(vec3());
            transform.set(matrix.ToFloatPtr());
            float rotation[4]={-90.0f,0,1,0};
            transform.postRotate(rotation);
            transform.get(matrix.ToFloatPtr());
            matrix.SetTranslation(projectile.m_position);
            transform.set(matrix.ToFloatPtr());
            IQueryInterface* model=m_model.m_interface;
            if (flight->m_guided) { model=m_alternateModel.m_interface; }
            graphics.render(model,transform.m_interface);
        }
    }
    m_smoke->Paint3D(graphics);
    m_flash->Paint3D(graphics);
}
void GrenadeLauncher::Simulate(float elapsed)
{
    Ballistics::Simulate(elapsed);
    m_smoke->Simulate(elapsed);
    m_flash->Simulate(elapsed);
    int count=m_activeCount;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active==false) { continue; }
        Flight* flight=(Flight*)projectile.m_userData;
        if (flight->m_guided && projectile.m_age>=0.25f) {
            projectile.m_velocity=projectile.m_velocity+flight->m_aimDirection*elapsed;
            if (flight->m_targetValid) {
                flight->m_aimDirection=Unit(flight->m_target-projectile.m_position)*Length(flight->m_aimDirection);
                float blend=projectile.m_age-0.25f;
                if (blend<0.0f) { blend=0.0f; }
                if (blend>1.0f) { blend=1.0f; }
                if (Length(projectile.m_position-flight->m_target)<25.0f) { blend=0.0f; }
                vec3 direction=Unit(projectile.m_velocity)*(1.0f-blend)+Unit(flight->m_aimDirection)*blend;
                float speed=Length(projectile.m_velocity);
                // Both source references alias the same pool entry in the original expression.
                projectile.m_velocity=projectile.m_velocity-projectile.m_velocity;
                projectile.m_velocity=projectile.m_velocity+Unit(direction)*speed;
            }
            vec3 direction=Unit(projectile.m_velocity);
            vec3 perpendicular=direction.Cross(Unit(vec3(direction.m_y,direction.m_z,direction.m_x)));
            vec3 up=direction.Cross(perpendicular);
            float amplitude=MathLib::Pow((float)CRandGen::GetInstance()->GetRandRange(0,1000)*0.001f,3.0f)*22.5f;
            vec3 wobble=perpendicular*MathLib::Sin(projectile.m_age*720.0f)+up*MathLib::Cos(projectile.m_age*720.0f);
            float blend=projectile.m_age;
            if (blend<0.0f) { blend=0.0f; }
            if (blend>1.0f) { blend=1.0f; }
            projectile.m_velocity=projectile.m_velocity+Unit(wobble)*amplitude*elapsed*blend;
            if (flight->m_flash2!=-1) {
                for (int flashIndex=0;flashIndex<m_flash->m_activeCount;flashIndex++) {
                    int active=m_flash->m_activeIndices[flashIndex];
                    if (active==flight->m_flash2) {
                        m_flash->m_projectiles[active].m_active=false;
                        m_flash->OnActivate(&m_flash->m_projectiles[active]);
                        break;
                    }
                }
            }
            flight->m_flash2=flight->m_flash1;
            flight->m_flash1=m_flash->AddProjectile(projectile.m_position-Unit(projectile.m_velocity)*0.2f,projectile.m_velocity,0.0f,0);
        }
        vec3 trailPosition=projectile.m_previous;
        vec3 direction=Unit(projectile.m_position-projectile.m_previous);
        float distance=Length(projectile.m_position-projectile.m_previous);
        float remaining=distance;
        if (!(distance+flight->m_trailDistance<m_trailSpacing)) {
            float first=m_trailSpacing-flight->m_trailDistance;
            trailPosition=trailPosition+direction*first;
            remaining=distance-first;
            m_smoke->AddProjectile(trailPosition,direction*(-m_trailVelocity),m_trailAge);
            flight->m_trailDistance=0;
            while (remaining>m_trailSpacing) {
                trailPosition=trailPosition+direction*m_trailSpacing;
                m_smoke->AddProjectile(trailPosition,direction*(-m_trailVelocity),m_trailAge);
                remaining-=m_trailSpacing;
            }
        } else { remaining=flight->m_trailDistance+distance; }
        flight->m_trailDistance=remaining;
        // The original accumulates the distance remaining after trail emission here.
        flight->m_distance+=remaining;
        if (flight->m_splitCount>1 && projectile.m_age>0.33f) {
            for (int split=0;split<flight->m_splitCount;split++) {
                AddProjectile(projectile.m_position,projectile.m_velocity*0.66f,15.0f,vec3(),false);
            }
            Projectile* original=&m_projectiles[m_activeIndices[index]];
            original->m_active=false;
            OnActivate(original);
            ((App*)WindowApp::m_instance)->m_soundManager->Play((CDH_SoundManager::eSound)1,false,false);
        }
    }
}
void GrenadeLauncher::IntersectProjectiles(HandGrenadeBallistics* grenades)
{
    int count=m_activeCount;
    int forwardIndex=0;
    while (count>0) {
        count--;
        Projectile& projectile=m_projectiles[m_activeIndices[count]];
        if (projectile.m_active) {
            CCollision::Line line;
            line.m_from=projectile.m_previous; line.m_to=projectile.m_position;
            int hit=0;
            if (grenades->Pick(line,hit)) {
                vec3 position=projectile.m_position;
                // The inclusive bound and the opposite traversal used for deactivation are original.
                if (hit>=0 && hit<=grenades->m_activeCount) {
                    Projectile& target=grenades->m_projectiles[grenades->m_activeIndices[hit]];
                    position=target.m_position;
                    vec3 right=Game()->GetCurrentCameraRight();
                    float vertical=(float)CRandGen::GetInstance()->GetRandRange(0,1000);
                    float horizontal=(float)CRandGen::GetInstance()->GetRandRange(-1000,1000);
                    vec3 perturbation=vec3(0,0,vertical*0.001f)+right*0.001f*horizontal;
                    target.m_velocity=target.m_velocity+Unit(perturbation)*Length(target.m_velocity)*0.2f;
                    position=position+Unit(projectile.m_position-projectile.m_previous)*0.5f;
                }
                App* app=(App*)WindowApp::m_instance;
                CDH_Weapon* weapon=app->m_helper->m_gamePlayManager.GetGameData()->GetActiveWeapon();
                Game()->BlastGrenade(position,vec3(0,0,1),m_explosionRadius,weapon,NULL);
                Projectile* removed=&m_projectiles[m_activeIndices[forwardIndex]];
                removed->m_active=false;
                OnActivate(removed);
            }
        }
        forwardIndex++;
    }
}
HandGrenadeBallistics::HandGrenadeBallistics(CCollision* collision)
    : Ballistics(16,30.0f,vec3(0,0,-9.8f),collision)
{
    m_flareScene=World(App::LoadObject3D("BIN_M3G_EFFECT_FLARE_SYSTEM"));
    m_flareRoot=Group(m_flareScene.find(DGHelper::getSwerveID("RootNode")));
    for (int index=0;index<8;index++) {
        XString name=XString::Format(L"Particle%02dTransform",index+1);
        m_flares[index]=Group(m_flareRoot.find(DGHelper::getSwerveID(XString::AnsiString(name.c_str()))));
    }
    {
        swerve::Object3D<CSwerveAssertingErrorHandler> auxiliary((IQueryInterface*)CSwerve::GetInstance()->LoadM3GResource("BIN_AUX_RENDER"));
        m_auxScene=World(auxiliary);
    }
    m_sphere=swerve::Mesh<CSwerveAssertingErrorHandler>(m_auxScene.find(DGHelper::getSwerveID("Sphere")));
    CGrenade* grenade=((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade();
    if (grenade==NULL) { m_normalBounce=0.1f; m_tangentRetention=0.75f; m_drag=0.125f; }
    else { m_normalBounce=grenade->m_bounce; m_tangentRetention=grenade->m_friction; m_drag=grenade->m_drag; }
}
HandGrenadeBallistics::~HandGrenadeBallistics()
{
    int count=m_activeCount;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_userData!=NULL) { delete (HandGrenade*)projectile.m_userData; }
    }
}
void HandGrenadeBallistics::OnPreSimulate(float) {} // Original return-only at 0xf05f4.
void HandGrenadeBallistics::AddProjectile(const vec3& position,const vec3& velocity,float spread,HandGrenade* grenade)
{
    grenade->m_rotationAxis.m_x=WindowApp::m_instance->GetRandomValue().CFloat();
    grenade->m_rotationAxis.m_y=WindowApp::m_instance->GetRandomValue().CFloat();
    grenade->m_rotationAxis.m_z=WindowApp::m_instance->GetRandomValue().CFloat();
    grenade->m_rotationAxis=Unit(grenade->m_rotationAxis);
    grenade->m_rotationSpeed=(WindowApp::m_instance->GetRandomValue().Float()*8.0f+8.0f)*Length(velocity);
    Ballistics::AddProjectile(position,velocity,spread,grenade);
}
int HandGrenadeBallistics::GetHitzoneUserId()
{
    static int userId=DGHelper::getSwerveID("GrenadeHitZoneTransform");
    return userId;
}
void HandGrenadeBallistics::OnActivate(Projectile* projectile)
{
    HandGrenade* grenade=(HandGrenade*)projectile->m_userData;
    if (projectile->m_active==false) {
        Game()->m_cameraNode.removeChild(grenade->GetVisualModel());
        delete grenade;
        projectile->m_userData=NULL;
    } else {
        Game()->m_cameraNode.addChild(grenade->GetVisualModel());
        Group hitzone(CSwerve::GetInstance()->m_swerve);
        hitzone.setUserID(GetHitzoneUserId());
        Transform transform(CSwerve::GetInstance()->m_swerve);
        transform.setIdentity();
        swerve::Mesh<CSwerveAssertingErrorHandler> sphere(m_sphere.duplicate());
        sphere.setUserID(GetHitzoneUserId());
        float radius=grenade->GetHitzoneRadius();
        float scale[3]={radius,radius,radius};
        transform.postScale(scale);
        hitzone.setTransform(transform);
        hitzone.setPickingEnable(true);
        sphere.setPickingEnable(true);
        sphere.setRenderingEnable(false);
        grenade->GetVisualModel().setPickingEnable(true);
        hitzone.addChild(sphere);
        grenade->GetVisualModel().addChild(hitzone);
    }
}
int HandGrenadeBallistics::OnCollision(Projectile* projectile,vec3* point,vec3* normal,CCollision::Triangle* triangle)
{
    if (triangle->m_meshIndex==-1) { projectile->m_velocity=vec3(); projectile->m_kinematic=false; }
    HandGrenade* grenade=(HandGrenade*)projectile->m_userData;
    grenade->m_normal=*normal;
    grenade->OnCollision(*point,*normal,*triangle);
    return 1;
}
bool HandGrenadeBallistics::Pick(const CCollision::Line& line,int& activeIndex)
{
    activeIndex=-1;
    int count=m_activeCount;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active) {
            HandGrenade* grenade=(HandGrenade*)projectile.m_userData;
            if (grenade==NULL) { continue; }
            CCollision::Sphere sphere;
            sphere.m_center=projectile.m_position;
            sphere.m_radius=grenade->GetHitzoneRadius();
            vec3 point;
            if (CCollision::Intersect(line,sphere,point)) {
                float cameraDistance=Length(projectile.m_position-Game()->GetCurrentCameraPos());
                float hitDistance=Length(point-line.m_from);
                float lineLength=Length(line.m_to-line.m_from);
                if (hitDistance<=lineLength) {
                    grenade->OnBulletHit(cameraDistance);
                    activeIndex=index;
                    return true;
                }
            }
        }
    }
    return false;
}
void HandGrenadeBallistics::Pick(float x,float y,swerve::Camera<CSwerveAssertingErrorHandler> camera)
{
    swerve::RayIntersection<CSwerveAssertingErrorHandler> intersection(CSwerve::GetInstance()->m_swerve);
    App* app=(App*)WindowApp::m_instance;
    int count=m_activeCount;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active && projectile.m_userData!=NULL) {
            HandGrenade* grenade=(HandGrenade*)projectile.m_userData;
            Group model=grenade->GetVisualModel();
            if (model.pick(-1,x/(float)app->m_helper->m_screenWidth,y/(float)app->m_helper->m_screenHeight,camera,intersection)) {
                grenade->OnBulletHit(Length(projectile.m_position-Game()->GetCurrentCameraPos()));
            }
        }
    }
}
HandGrenadeBallistics::FragGrenade::FragGrenade()
{
    CGrenade* grenade=((App*)WindowApp::m_instance)->m_grenadeManager->GetActiveGrenade();
    World scene(App::LoadObject3D(grenade->m_model));
    m_model=Group(scene.find(DGHelper::getSwerveID("RootNode")));
    m_delay=(float)grenade->m_delayAfterContact*0.001f;
    m_contacted=false; m_exploded=false; m_collisionCount=0;
}
HandGrenadeBallistics::FragGrenade::~FragGrenade() {}
Group HandGrenadeBallistics::FragGrenade::GetVisualModel() { return m_model; }
void HandGrenadeBallistics::FragGrenade::OnBulletHit(float) { m_delay=0; m_contacted=true; }
int HandGrenadeBallistics::FragGrenade::OnCollision(const vec3&,const vec3&,const CCollision::Triangle&)
{
    m_contacted=true; m_collisionCount++; return 1;
}
bool HandGrenadeBallistics::FragGrenade::IsExploded() { return m_exploded; }
void HandGrenadeBallistics::FragGrenade::OnUpdate(float elapsed,const Projectile*)
{
    App* app=(App*)WindowApp::m_instance;
    CGrenade* grenade=app->m_grenadeManager->GetActiveGrenade();
    if (grenade->m_isBeacon && app->m_soundManager->IsPlaying((CDH_SoundManager::eSound)0x7c)==0) {
        app->m_soundManager->Play((CDH_SoundManager::eSound)0x7c,false,false);
    }
    if (m_contacted) {
        m_delay-=elapsed;
        if (m_delay<=0.0f && m_exploded==false) {
            m_exploded=true;
            if (grenade->m_damagePeripheral==0) {
                vec3 position;
                m_model.getTranslation(&position.m_x);
                vec3 direction=Unit(Game()->GetCurrentCameraPos()-position);
                Game()->AddFlash(position+direction*0.1f,direction*0.01f,0.0f);
                app->m_soundManager->Play((CDH_SoundManager::eSound)0x62,false,false);
            } else {
                int centerDamage=grenade->m_damageCenter;
                int peripheralDamage=grenade->m_damagePeripheral;
                int radius=grenade->m_explosionRadius;
                CUnitsController* controller=Game()->GetUnitsController();
                float shardNumber=grenade->m_shardCount;
                float speed=grenade->m_shardSpeed;
                vec3 position;
                m_model.getTranslation(&position.m_x);
                Game()->Blast(position,vec3(0,0,1),(float)radius,centerDamage,peripheralDamage,NULL,grenade,NULL);
                app->m_soundManager->Play((CDH_SoundManager::eSound)0,false,false);
                int shardCount=(int)shardNumber;
                position=position+vec3(0,0,0.25f);
                if (shardCount!=0) {
                    Vector<vec3> targets;
                    const Vector<CUnit*>& units=controller->GetUnits();
                    int unitCount=units.size();
                    for (int unitIndex=0;unitIndex<unitCount;unitIndex++) {
                        CUnit* unit=units[unitIndex];
                        if (unit->IsCivilian()==false) {
                            vec3 target=unit->GetPosition();
                            float distance=Length(target-position);
                            int insert=0;
                            for (;insert<targets.size();insert++) {
                                if (distance<Length(targets[insert]-position)) { break; }
                            }
                            if (insert<targets.size()) { targets.insertElementAt(target,insert); }
                            else { targets.addElement(target); }
                        }
                    }
                    int targetIndex=0;
                    for (int index=0;index<shardCount/2;index++) {
                        vec3 velocity;
                        float spread;
                        if (((index&1)==0 || m_collisionCount==0) && targetIndex<targets.size()) {
                            vec3 target=targets[targetIndex]+vec3(0,0,1);
                            targetIndex++;
                            velocity=Unit(position-target)*speed;
                            spread=0.7f;
                        } else { velocity=vec3(0,0,1)*speed; spread=90.0f; }
                        Game()->AddRicochet(position,velocity,spread,1);
                        Game()->AddRicochet(position,vec3()-velocity,spread,1);
                    }
                }
            }
        }
    }
}
HandGrenadeBallistics::StoneGrenade::StoneGrenade(Group model,int hp)
{
    // Original: m_model default-constructed, then assigned (zero store + operator=).
    m_model=model;
    m_hp=hp; m_exploded=false; m_hitWorld=false; m_lighting=NULL;
    if (CDH_Animation::HaveController(model,"lighting")) {
        m_lighting=new CDH_Animation(model);
        m_lighting->addController(XString("lighting"));
        m_lighting->setCurrentController(0,false);
        m_lighting->setRelativePositionAndStop(WindowApp::m_instance->GetRandomValue().Float()*0.5f);
    }
}
HandGrenadeBallistics::StoneGrenade::~StoneGrenade()
{
    if (m_lighting!=NULL) { delete m_lighting; m_lighting=NULL; }
}
Group HandGrenadeBallistics::StoneGrenade::GetVisualModel() { return m_model; }
void HandGrenadeBallistics::StoneGrenade::OnBulletHit(float distance)
{
    m_hp-=((App*)WindowApp::m_instance)->m_helper->GetGameData()->GetActiveWeapon()->GetShotDamage(distance);
    if (m_hp<0) { m_hitWorld=true; m_exploded=true; }
}
void HandGrenadeBallistics::StoneGrenade::SetLuminance(float brightness)
{
    if (m_lighting!=NULL) {
        float value=0.5f;
        if (brightness>=0.0f && brightness<=1.0f) { value=brightness; }
        m_lighting->setRelativePositionAndStop(1.0f-value);
    }
}
int HandGrenadeBallistics::StoneGrenade::OnCollision(const vec3&,const vec3&,const CCollision::Triangle& triangle)
{
    m_exploded=true;
    if (triangle.m_meshIndex!=-1) { m_hitWorld=true; }
    return 1;
}
void HandGrenadeBallistics::StoneGrenade::OnUpdate(float,const Projectile*)
{
    if (m_exploded) {
        if (m_hitWorld==false) { Game()->OnShooterAttacked(); }
        else {
            Transform transform(CSwerve::GetInstance()->m_swerve);
            m_model.getTransformTo(Game()->m_cameraNode,transform);
            // Original: raw float[16] (no vec4 zeroing), translation read inline.
            float matrix[16];
            transform.get(matrix);
            Game()->GetDGWorld()->stoneBlast(vec3(matrix[3],matrix[7],matrix[11]),vec3(0,0,1));
        }
    }
}
bool HandGrenadeBallistics::StoneGrenade::IsExploded() { return m_exploded; }
void HandGrenadeBallistics::PostSimulate()
{
    int count=m_activeCount;
    CCollision::AABB bounds=m_collision->m_root->m_bounds;
    vec3 overhead=bounds.m_min+(bounds.m_max-bounds.m_min)*0.5f+vec3(0,0,1000);
    bounds.Add(overhead);
    float elapsed=WindowApp::m_instance->m_frameTimeSeconds;
    vec3 camera=Game()->GetCurrentCameraPos();
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active==false) { continue; }
        HandGrenade* grenade=(HandGrenade*)projectile.m_userData;
        if (grenade->HasPlayerRelativeDamping()) {
            vec3 outward=Unit(projectile.m_position-camera);
            float toward=-outward.Dot(projectile.m_velocity);
            if (toward>0.0f) {
                float blend=(Length(camera-projectile.m_position)-10.0f)/15.0f;
                if (blend<0.0f) { blend=0.0f; }
                if (blend>1.0f) { blend=1.0f; }
                projectile.m_velocity=projectile.m_velocity+outward*(1.0f-blend)*toward*elapsed;
            }
        }
        if (projectile.m_age>1.0f) {
            const vec3& position=projectile.m_position;
            if (!(position.m_x>=bounds.m_min.m_x) || !(position.m_x<=bounds.m_max.m_x) ||
                !(position.m_y>=bounds.m_min.m_y) || !(position.m_y<=bounds.m_max.m_y) ||
                !(position.m_z>=bounds.m_min.m_z) || !(position.m_z<=bounds.m_max.m_z)) {
                grenade->OnBulletHit(Length(position-Game()->GetCurrentCameraPos()));
            }
        }
        vec3 position=projectile.m_position+grenade->m_normal*0.05f;
        grenade->GetVisualModel().setTranslation(&position.m_x);
        float rotation[4]={elapsed*grenade->m_rotationSpeed,grenade->m_rotationAxis.m_x,grenade->m_rotationAxis.m_y,grenade->m_rotationAxis.m_z};
        grenade->GetVisualModel().postRotate(rotation);
        grenade->SetLuminance(Game()->GetSceneLighting()->GetLuminance(position));
        if (grenade->IsDynamicallyScaled()) {
            float distance=Length(Game()->GetCurrentCameraPos()-projectile.m_position)/100.0f;
            if (distance>1.0f) { distance=1.0f; }
            float scale=(1.0f-distance)+distance*2.0f;
            float scaling[3]={scale,scale,scale};
            grenade->GetVisualModel().setScale(scaling);
        }
        grenade->OnUpdate(elapsed,&projectile);
        if (grenade->IsExploded()) {
            projectile.m_active=false;
            OnActivate(&projectile);
        }
    }
}
void HandGrenadeBallistics::Paint3D(swerve::Graphics3D<CSwerveAssertingErrorHandler>& graphics)
{
    int count=m_activeCount;
    vec3 camera=Game()->GetCurrentCameraPos();
    Game()->m_camera->GetViewDirection();
    Game()->GetCurrentCameraRight();
    vec3 up=Game()->GetCurrentCameraUp();
    Mat4x4 matrix(1.0f);
    Transform renderTransform(CSwerve::GetInstance()->m_swerve);
    renderTransform.setIdentity();
    Transform basis(CSwerve::GetInstance()->m_swerve);
    Transform scratch(CSwerve::GetInstance()->m_swerve);
    for (int index=0;index<8;index++) { m_flares[index].setRenderingEnable(false); }
    int batch=0;
    for (int index=0;index<count;index++) {
        Projectile& projectile=m_projectiles[m_activeIndices[index]];
        if (projectile.m_active==false) { continue; }
        HandGrenade* grenade=(HandGrenade*)projectile.m_userData;
        vec3 center=projectile.m_position-Unit(projectile.m_velocity)*0.05f;
        vec3 view=center-camera;
        float fade=(100.0f-Length(view))/100.0f;
        if (fade<0.0f) { fade=0.0f; }
        if (fade>1.0f) { fade=1.0f; }
        float flicker=MathLib::Sin(projectile.m_age*1718.8734f);
        vec3 front=Unit(view);
        vec3 axis=Unit(up);
        vec3 side=Unit(axis.Cross(front));
        float scale=(fade*0.5f+(1.0f-fade)*4.0f)*0.5f*(flicker+1.0f);
        vec3 first=axis*50.0f*scale*Length(up);
        vec3 second=side*50.0f*scale;
        vec3 third=front*50.0f*scale*Length(up);
        float* values=matrix.ToFloatPtr();
        values[0]=first.m_x; values[4]=first.m_y; values[8]=first.m_z;
        values[1]=second.m_x; values[5]=second.m_y; values[9]=second.m_z;
        values[2]=third.m_x; values[6]=third.m_y; values[10]=third.m_z;
        matrix.SetTranslation(vec3());
        basis.set(values);
        scratch.setIdentity();
        float rotation[4]={0.0f,third.m_x,third.m_y,third.m_z};
        scratch.postRotate(rotation);
        scratch.postMultiply(basis);
        basis.multiply(scratch);
        scratch.setIdentity();
        scratch.postTranslate(&center.m_x);
        scratch.postMultiply(basis);
        m_flares[batch].setTransform(scratch);
        if (grenade->HasIgniteFlare()) { m_flares[batch].setRenderingEnable(true); }
        float age=projectile.m_age;
        if (age>1.0f) { age=1.0f; }
        m_flares[batch].setAlphaFactor(MathLib::Pow(age,2.0f));
        batch++;
        if (batch==8) {
            graphics.render(m_flareRoot.m_interface,renderTransform.m_interface);
            for (int hidden=0;hidden<8;hidden++) { m_flares[hidden].setRenderingEnable(false); }
            batch=0;
        }
    }
    if (batch>0) { graphics.render(m_flareRoot.m_interface,renderTransform.m_interface); }
}
