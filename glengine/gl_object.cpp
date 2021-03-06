#include "gl_object.h"
#include "gl_camera.h"
#include "gl_mesh.h"
#include "gl_logger.h"

#include "microprofile/microprofile.h"

#include <vector>
#include <set>

namespace glengine {

Object::Object(Object *parent, ID id)
: _id(id)
, _parent(parent) {
    if (_parent) {
        parent->add_child(this);
    }
}

Object::~Object() {
    log_debug("Destroying object %p",this);
    if (_parent) {
        _parent->detach_child(this);
    }
    // have to iterate this way because a regular iterator gets invalidated when deleting
    while (_children.size() > 0) {
        delete *_children.begin();
    }
}

bool Object::init(const std::vector<Renderable> &renderables) {
    _renderables = renderables;
    return true;
}

bool Object::add_renderable(const Renderable *r, uint32_t num) {
    _renderables.insert(_renderables.end(), r, r + num);
    return true;
}

void Object::update() {
    for (auto &r : _renderables) {
        r.update();
    }
}

void Object::update_bindings() {
    for (auto &r : _renderables) {
        r.update_bindings();
    }
}

void Object::add_child(Object *ro) {
    if (!ro) {
        return;
    }
    // update previous parent
    if (ro->parent()) {
        ro->parent()->detach_child(ro);
    }
    ro->_parent = this;
    _children.insert(ro);
}

Object *Object::detach_child(Object *ro) {
    auto it = _children.find(ro);
    if (it != _children.end()) {
        Object *orphan = *it;
        orphan->_parent = nullptr;
        _children.erase(it);
        return orphan;
    }
    return nullptr;
}

bool Object::draw(const Camera &cam, const math::Matrix4f &parent_tf) {
    // MICROPROFILE_SCOPEI("renderobject", "draw", MP_AUTO);
    math::Matrix4f curr_tf = parent_tf * _transform * _scale;
    if (_visible) {
        for (auto &go : _renderables) {
            // MICROPROFILE_SCOPEI("renderobject", "render_renderables", MP_AUTO);
            // renderer.render_items.push_back({&cam, &go, curr_tf, _id});
            go.apply_pipeline();
            go.apply_bindings();
            glengine::common_uniform_params_t obj_params;
            obj_params.model = curr_tf;
            obj_params.view = cam.inverse_transform();
            obj_params.projection = cam.projection();
            go.apply_uniforms(obj_params);
            go.draw();
        }
        for (auto &c : _children) {
            c->draw(cam, curr_tf);
        }
    }
    return true;
}

Object &Object::set_transform(const math::Matrix4f &tf) {
    _transform = tf;
    return *this;
}
Object &Object::set_scale(const math::Vector3f &scl) {
    _scale(0, 0) = scl[0];
    _scale(1, 1) = scl[1];
    _scale(2, 2) = scl[2];
    return *this;
}
Object &Object::set_visible(bool flag) {
    _visible = flag;
    return *this;
}

} // namespace glengine
