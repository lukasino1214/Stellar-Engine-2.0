#include <data/components.hpp>

#include <cstring>

#include <utils/gui.hpp>

namespace Stellar {
    void UUIDComponent::draw() {
        GUI::begin_properties();

        GUI::push_deactivated_status();
        GUI::u64_property("UUID:", uuid.uuid, nullptr, ImGuiInputTextFlags_ReadOnly);
        GUI::pop_deactivated_status();

        GUI::end_properties();
    }

    void TagComponent::draw() {
        GUI::begin_properties();
        GUI::string_property("Tag:", name, nullptr, ImGuiInputTextFlags_None);
        GUI::end_properties();
    }

    void RelationshipComponent::draw() {

    }

    void TransformComponent::draw() {
        GUI::begin_properties(ImGuiTableFlags_BordersInnerV);

        static std::array<f32, 3> reset_values = { 0.0f, 0.0f, 0.0f };
        static std::array<const char*, 3> tooltips = { "Some tooltip.", "Some tooltip.", "Some tooltip." };

        if (GUI::vec3_property("Position:", position, reset_values.data(), tooltips.data())) { is_dirty = true;}
        if (GUI::vec3_property("Rotation:", rotation, reset_values.data(), tooltips.data())) { is_dirty = true;}

        reset_values = { 1.0f, 1.0f, 1.0f };

        if (GUI::vec3_property("Scale:", scale, reset_values.data(), tooltips.data())) { is_dirty = true;}

        GUI::end_properties();
    }
}