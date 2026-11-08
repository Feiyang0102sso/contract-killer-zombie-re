// CDH_Animation.cpp - named animation controllers of one Swerve object.
#include "CDH_Animation.h"
#include "DGHelper.h"
#include "WindowApp.h"

CDH_Animation::CDH_Animation(Object3D object)
    : m_object(object)
{
    m_currentController = -1;
    m_startTime = -1;
}

CDH_Animation::~CDH_Animation()
{
}

int getControllerAnimDuration(swerve::Object3D<CSwerveAssertingErrorHandler> object,
                              swerve::AnimationController<CSwerveAssertingErrorHandler> controller)
{
    int maxDuration = 0;

    // tracks of the object itself
    int trackCount = object.getAnimationTrackCount();
    for (int index = trackCount - 1; index >= 0; index--) {
        swerve::AnimationTrack<CSwerveAssertingErrorHandler> track = object.getAnimationTrack(index);
        if (track.getController() == controller) {
            int duration = track.getKeyframeSequence().getDuration();
            if (maxDuration < duration) {
                maxDuration = duration;
            }
        }
    }

    // children of a group
    swerve::Group<CSwerveAssertingErrorHandler> group(object);
    if (group != NULL) {
        int childCount = group.getChildCount();
        for (int index = childCount - 1; index >= 0; index--) {
            int duration = getControllerAnimDuration(group.getChild(index), controller);
            if (maxDuration < duration) {
                maxDuration = duration;
            }
        }
    }

    // appearances (or index buffers) of a mesh
    swerve::Mesh<CSwerveAssertingErrorHandler> mesh(object);
    if (mesh != NULL) {
        int submeshCount = mesh.getSubmeshCount();
        for (int index = submeshCount - 1; index >= 0; index--) {
            swerve::Appearance<CSwerveAssertingErrorHandler> appearance = mesh.getAppearance(index);
            int duration;
            if (appearance != NULL) {
                duration = getControllerAnimDuration(appearance, controller);
            } else {
                duration = getControllerAnimDuration(mesh.getIndexBuffer(index), controller);
            }
            if (maxDuration < duration) {
                maxDuration = duration;
            }
        }
    }

    // skeleton of a skinned mesh
    swerve::SkinnedMesh<CSwerveAssertingErrorHandler> skinnedMesh(object);
    if (skinnedMesh != NULL) {
        swerve::Group<CSwerveAssertingErrorHandler> skeleton = skinnedMesh.getSkeleton();
        int duration = getControllerAnimDuration(skeleton, controller);
        if (maxDuration < duration) {
            maxDuration = duration;
        }
    }

    // the two textures of an appearance
    swerve::Appearance<CSwerveAssertingErrorHandler> appearance(object);
    if (appearance != NULL) {
        for (int index = 0; index < 2; index++) {
            swerve::Texture2D<CSwerveAssertingErrorHandler> texture = appearance.getTexture(index);
            if (texture != NULL) {
                int duration = getControllerAnimDuration(texture, controller);
                if (maxDuration < duration) {
                    maxDuration = duration;
                }
            }
        }
    }
    return maxDuration;
}

CDH_Animation::AnimationTrack CDH_Animation::getFirstControlledTrack(Object3D object, AnimationController controller)
{
    // tracks of the object itself
    int trackCount = object.getAnimationTrackCount();
    for (int index = trackCount - 1; index >= 0; index--) {
        AnimationTrack track = object.getAnimationTrack(index);
        if (track.getController() == controller) {
            return track;
        }
    }

    // children of a group
    swerve::Group<CSwerveAssertingErrorHandler> group(object);
    if (group != NULL) {
        int childCount = group.getChildCount();
        for (int index = childCount - 1; index >= 0; index--) {
            AnimationTrack track = getFirstControlledTrack(group.getChild(index), controller);
            if (track != NULL) {
                return track;
            }
        }
    }

    // appearances (or index buffers) of a mesh
    swerve::Mesh<CSwerveAssertingErrorHandler> mesh(object);
    if (mesh != NULL) {
        int submeshCount = mesh.getSubmeshCount();
        for (int index = submeshCount - 1; index >= 0; index--) {
            AnimationTrack track;
            swerve::Appearance<CSwerveAssertingErrorHandler> appearance = mesh.getAppearance(index);
            if (appearance != NULL) {
                track = getFirstControlledTrack(appearance, controller);
            } else {
                track = getFirstControlledTrack(mesh.getIndexBuffer(index), controller);
            }
            if (track != NULL) {
                return track;
            }
        }
    }

    // skeleton of a skinned mesh
    swerve::SkinnedMesh<CSwerveAssertingErrorHandler> skinnedMesh(object);
    if (skinnedMesh != NULL) {
        swerve::Group<CSwerveAssertingErrorHandler> skeleton = skinnedMesh.getSkeleton();
        AnimationTrack track = getFirstControlledTrack(skeleton, controller);
        if (track != NULL) {
            return track;
        }
    }

    // the two textures of an appearance
    swerve::Appearance<CSwerveAssertingErrorHandler> appearance(object);
    if (appearance != NULL) {
        for (int index = 0; index < 2; index++) {
            swerve::Texture2D<CSwerveAssertingErrorHandler> texture = appearance.getTexture(index);
            if (texture != NULL) {
                AnimationTrack track = getFirstControlledTrack(texture, controller);
                if (track != NULL) {
                    return track;
                }
            }
        }
    }
    return AnimationTrack();
}

void CDH_Animation::addController(const XString& name)
{
    AnimationController controller = m_object.find(DGHelper::getSwerveID(XString::AnsiString(name.m_data)));
    controller.setWeight(0.0f);
    int trackDuration = getFirstControlledTrack(m_object, controller).getKeyframeSequence().getDuration();
    int animationDuration = getControllerAnimDuration(m_object, controller);
    int duration = trackDuration;
    if (trackDuration < animationDuration) {
        duration = animationDuration;
    }
    m_controllers.addElement(controller);
    m_durations.addElement(duration);
}

int CDH_Animation::getControllerBaseDuration(int index)
{
    return m_durations[index];
}

void CDH_Animation::setControllerDuration(int index, int duration)
{
    float speed = (float)m_durations[index] / (float)duration;
    m_controllers[index].setSpeed(speed, WindowApp::m_instance->m_updateTime);
}

void CDH_Animation::setControllerSpeed(int index, float speed)
{
    if (index >= 0 && index < m_controllers.size()) {
        int time = WindowApp::m_instance->m_updateTime;
        AnimationController controller = m_controllers[index];
        controller.setSpeed(speed, time);
    }
}

float CDH_Animation::getControllerSpeed(int index)
{
    if (index >= 0 && index < m_controllers.size()) {
        AnimationController controller = m_controllers[index];
        return controller.getSpeed();
    }
    return 1.0f;
}

float CDH_Animation::getControllerPosition(int index)
{
    if (index >= 0 && index < m_controllers.size()) {
        int time = WindowApp::m_instance->m_updateTime;
        AnimationController controller = m_controllers[index];
        return controller.getPosition(time) / (float)m_durations[index];
    }
    return 0.0f;
}

void CDH_Animation::setControllerPosition(int index, float position)
{
    if (index >= 0 && index < m_controllers.size()) {
        int time = WindowApp::m_instance->m_updateTime;
        AnimationController controller = m_controllers[index];
        controller.setPosition((float)m_durations[index] * position, time);
    }
}

void CDH_Animation::setCurrentController(int index, bool reverse)
{
    int time = WindowApp::m_instance->m_updateTime;
    stopAllAnimation();
    if (index >= 0 && index < m_controllers.size()) {
        AnimationController controller = m_controllers[index];
        controller.setWeight(1.0f);
        float speed = controller.getSpeed();
        if (speed < 0.0f) {
            speed = -speed;
        }
        if (reverse) {
            // play backwards from the end
            controller.setPosition((float)m_durations[index], time);
            controller.setSpeed(-speed, time);
        } else {
            controller.setPosition(0.0f, time);
            controller.setSpeed(speed, time);
        }
    }
    m_currentController = index;
    // ?: gives the original "cmp #-1; movle" (an if gives "cmp #0; movlt")
    m_startTime = (index >= 0) ? time : -1;
}

void CDH_Animation::stopAllAnimation()
{
    for (int index = 0; index < m_controllers.size(); index++) {
        m_controllers[index].setWeight(0.0f);
    }
}

void CDH_Animation::animate()
{
    m_object.animate(WindowApp::m_instance->m_updateTime);
}

void CDH_Animation::setRelativePosition(float position)
{
    for (int index = 0; index < m_controllers.size(); index++) {
        m_controllers[index].setPosition((float)m_durations[index] * position, WindowApp::m_instance->m_updateTime);
    }
}

void CDH_Animation::setRelativePositionAndStop(float position)
{
    for (int index = 0; index < m_controllers.size(); index++) {
        m_controllers[index].setPosition((float)m_durations[index] * position, WindowApp::m_instance->m_updateTime);
        m_controllers[index].setSpeed(0.0f, WindowApp::m_instance->m_updateTime);
    }
}

bool CDH_Animation::isAnimationComplete() const
{
    if (m_currentController < 0) {
        return true;
    }
    int duration = m_durations[m_currentController];
    int elapsed = WindowApp::m_instance->m_updateTime - m_startTime;
    float speed = m_controllers[m_currentController].getSpeed();
    return (float)elapsed >= (float)duration / speed;
}

bool CDH_Animation::HaveController(Object3D object, const char* name)
{
    AnimationController controller = object.find(DGHelper::getSwerveID(name));
    return controller != NULL;
}
