/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ClientData.hpp"
#include "CommandList.hpp"
#include "World.hpp"
#include <Helpers/NetworkingObjects/NetObjGenericSerializedClass.hpp>
#include "MainProgram.hpp"
#include "ScaleUpCanvas.hpp"

#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skparagraph/include/ParagraphStyle.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/skparagraph/include/TextStyle.h>
#include <modules/skunicode/include/SkUnicode_icu.h>

using namespace NetworkingObjects;

enum class ClientDataCommand : uint8_t {
    SET_CURSOR_POS = 0,
    SET_WINDOW_SIZE,
    SET_CAM_COORDS,
    SEND_CHAT_MESSAGE,
    SET_GRID_SIZE
};

ClientData::ClientData() {
    set_from_init_struct(InitStruct());
}

ClientData::ClientData(const InitStruct& initStruct) {
    set_from_init_struct(initStruct);
}

void ClientData::set_from_init_struct(const InitStruct& initStruct) {
    camCoords = initStruct.camCoords;
    windowSize = initStruct.windowSize;
    cursorPos = initStruct.cursorPos;
    cursorColor = initStruct.cursorColor;
    displayName = initStruct.displayName;
    gridSize = initStruct.gridSize;
}

void ClientData::register_class(World& world) {
    world.netObjMan.register_class<ClientData, ClientData, ClientData, ClientData>({
        .writeConstructorFuncClient = default_serialize_write_func<ClientData>,
        .readConstructorFuncClient = default_serialize_read_constructor_func<ClientData>,
        .readUpdateFuncClient = [&world](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
            ClientDataCommand command; 
            a(command);
            switch(command) {
                case ClientDataCommand::SET_CURSOR_POS:
                    a(o->cursorPos);
                    break;
                case ClientDataCommand::SET_WINDOW_SIZE:
                    a(o->windowSize);
                    break;
                case ClientDataCommand::SET_CAM_COORDS:
                    a(o->camCoords);
                    break;
                case ClientDataCommand::SEND_CHAT_MESSAGE: {
                    std::string chatMessage;
                    a(chatMessage);
                    world.add_chat_message(o->displayName, chatMessage, Toolbar::ChatMessage::Type::NORMAL);
                    break;
                }
                case ClientDataCommand::SET_GRID_SIZE: {
                    uint32_t oldGridSize = o->gridSize;
                    a(o->gridSize);
                    if(oldGridSize < o->gridSize)
                        world.scale_up(get_canvas_scale_up_amount(o->gridSize, oldGridSize));
                    break;
                }
            }
        },
        .writeConstructorFuncServer = default_serialize_write_func<ClientData>,
        .readConstructorFuncServer = default_serialize_read_constructor_func<ClientData>,
        .readUpdateFuncServer = [&world](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
            ClientDataCommand command; 
            a(command);
            switch(command) {
                case ClientDataCommand::SET_CURSOR_POS:
                    a(o->cursorPos);
                    o.send_server_update_to_all_clients_except(c, UNRELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
                        a(ClientDataCommand::SET_CURSOR_POS, o->cursorPos);
                    });
                    break;
                case ClientDataCommand::SET_WINDOW_SIZE:
                    a(o->windowSize);
                    o.send_server_update_to_all_clients_except(c, RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
                        a(ClientDataCommand::SET_WINDOW_SIZE, o->windowSize);
                    });
                    break;
                case ClientDataCommand::SET_CAM_COORDS:
                    a(o->camCoords);
                    o.send_server_update_to_all_clients_except(c, RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
                        a(ClientDataCommand::SET_CAM_COORDS, o->camCoords);
                    });
                    break;
                case ClientDataCommand::SEND_CHAT_MESSAGE: {
                    std::string chatMessage;
                    a(chatMessage);
                    world.add_chat_message(o->displayName, chatMessage, Toolbar::ChatMessage::Type::NORMAL);
                    o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&chatMessage](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
                        a(ClientDataCommand::SEND_CHAT_MESSAGE, chatMessage);
                    });
                    break;
                }
                case ClientDataCommand::SET_GRID_SIZE: {
                    uint32_t oldGridSize = o->gridSize;
                    a(o->gridSize);
                    if(oldGridSize < o->gridSize)
                        world.scale_up(get_canvas_scale_up_amount(o->gridSize, oldGridSize));
                    o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
                        a(ClientDataCommand::SET_GRID_SIZE, o->gridSize);
                    });
                    break;
                }
            }
        },
    });
}

void ClientData::set_cursor_pos(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, Vector2f newPos) {
    o->cursorPos = newPos;
    o.send_update_to_all(UNRELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
        a(ClientDataCommand::SET_CURSOR_POS, o->cursorPos);
    });
}

void ClientData::set_window_size(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, Vector2f newWindowSize) {
    if(o->windowSize != newWindowSize) {
        o->windowSize = newWindowSize;
        o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
            a(ClientDataCommand::SET_WINDOW_SIZE, o->windowSize);
        });
    }
}

void ClientData::set_camera_coords(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, const CoordSpaceHelper& newCoords) {
    if(o->camCoords != newCoords) {
        o->camCoords = newCoords;
        o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
            a(ClientDataCommand::SET_CAM_COORDS, o->camCoords);
        });
    }
}

void ClientData::send_chat_message(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, World& world, const std::string& chatMessage) {
    if(o.get_obj_man()->is_server()) // Clients will receive the message when it returns from the server to ensure correct order
        world.add_chat_message(o->displayName, chatMessage, Toolbar::ChatMessage::Type::NORMAL);
    o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, [&chatMessage](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
        a(ClientDataCommand::SEND_CHAT_MESSAGE, chatMessage);
    });
}

void ClientData::scale_up_step(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, World& world) {
    o->gridSize++;
    world.scale_up(CANVAS_SCALE_UP_STEP);
    o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<ClientData>& o, cereal::PortableBinaryOutputArchive & a) {
        a(ClientDataCommand::SET_GRID_SIZE, o->gridSize);
    });
}

void ClientData::set_display_name(const std::string& newDisplayName) {
    displayName = newDisplayName;
}

void ClientData::set_cursor_color(const Vector3f& newCursorColor) {
    cursorColor = newCursorColor;
}

const CoordSpaceHelper& ClientData::get_cam_coords() const {
    return camCoords;
}

const Vector2f& ClientData::get_window_size() const {
    return windowSize;
}

const Vector2f& ClientData::get_cursor_pos() const {
    return cursorPos;
}

uint32_t ClientData::get_grid_size() const {
    return gridSize;
}

const std::string& ClientData::get_display_name() const {
    return displayName;
}

const Vector3f& ClientData::get_cursor_color() const {
    return cursorColor;
}

void ClientData::draw_cursor(SkCanvas* canvas, const DrawData& drawData) const {
    if((drawData.cam.c.inverseScale << 10) > camCoords.inverseScale && drawData.clampDrawMinimum < camCoords.inverseScale) {
        canvas->save();
        camCoords.transform_sk_canvas(canvas, drawData);

        canvas->translate(cursorPos.x(), cursorPos.y());

        {
            SkPaint p(SkColor4f{cursorColor.x(), cursorColor.y(), cursorColor.z(), 0.3f});
            p.setAntiAlias(drawData.skiaAA);
            canvas->drawCircle(0.0f, 0.0f, 4.5f, p);
            p.setStroke(true);
            p.setStrokeWidth(1.0f);
            p.setColor4f({cursorColor.x(), cursorColor.y(), cursorColor.z(), 0.6f});
            canvas->drawCircle(0.0f, 0.0f, 5.0f, p);
        }

        Vector2f bounds;
        std::unique_ptr<skia::textlayout::Paragraph> paragraph; 
        {
            skia::textlayout::ParagraphStyle pStyle;
            pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
            skia::textlayout::TextStyle tStyle;
            tStyle.setFontSize(18.0f);
            tStyle.setFontFamilies(drawData.main->g.gui.io.fonts->get_default_font_families());
            tStyle.setForegroundPaint(SkPaint{SkColor4f{cursorColor.x(), cursorColor.y(), cursorColor.z(), 0.6f}});
            pStyle.setTextStyle(tStyle);

            skia::textlayout::ParagraphBuilderImpl a(pStyle, drawData.main->g.gui.io.fonts->collection, SkUnicodes::ICU::Make());
            a.addText(displayName.c_str(), displayName.length());
            paragraph = a.Build();
            paragraph->layout(std::numeric_limits<float>::max());
            
            bounds = {paragraph->getMaxIntrinsicWidth(), paragraph->getHeight()};
        }

        Vector2f textPos = {6.0f, -bounds.y()};

        SkPaint p3(color_mul_alpha(drawData.main->g.gui.io.theme->backColor1, 0.6f));
        p3.setAntiAlias(drawData.skiaAA);
        canvas->drawRoundRect(SkRect::MakeXYWH(textPos.x(), textPos.y(), bounds.x(), bounds.y()), 3.0f, 3.0f, p3);

        paragraph->paint(canvas, textPos.x(), textPos.y());

        canvas->restore();
    }
}
