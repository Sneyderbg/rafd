from math import atan2, cos, degrees, pi, radians, sin
from typing import Tuple, TypedDict
from utils import Vec2

import pyray as pr

import afd1
from afd import AFD


class Node(TypedDict):
    pos: Vec2
    vel: Vec2
    is_target: bool


FONT_SIZE = 24
SIZE = (1280, 720)
FPS = 100
BACKGROUND_COLOR = pr.RAYWHITE

# -------------------------
# -------- GLOBALS --------
# -------------------------

bg_texture = pr.Texture()
bg_frag_shader = pr.Shader()

font_path = "./fonts/CodeSquaredRegular-AYRg.ttf"
font = pr.get_font_default()

show_help = False
help_hint_timer = 4  # secs
help_info = """
H          - Show this help
Enter      - Start typing a new input for the afd
Esc        - Cancel typing
Left click - Drag nodes
"""

# edge style
PADDING_FROM_NODE = 10
SEP_FACTOR = 20
TRIANGLE_SIZE = 20
THICKNESS = 2
EDGE_TAG_SCALE = 0.6
TAG_SEPARATION = 20

dragging_node = False
mouse_over_node = None

edge_force = 5.0
node_repulsion = 2e6
deccel = 4.0
force_length_start = 4
"from when to apply the respective forces in max_r units"

typing_str = False
feeding_str = ""
feeding = False
feeding_idx = 0
feeding_timer = 0
feeding_delay = 2  # secs
feeding_finished = False

live_mode = pr.ffi.new("bool *", False)  # TODO: implement: feed while typing

msg = None


def init():
    global running, camera2D, nodes, automata, n, font, max_r, name_lens

    # setup automata
    automata = AFD(
        afd1.original["sigma"],
        afd1.original["Q"],
        afd1.original["F"],
        afd1.original["delta"],
    )
    n = automata.n_nodes

    nodes = {
        state: Node(
            {
                "pos": Vec2(pr.get_random_value(0, n), pr.get_random_value(0, n)),
                "is_target": state in automata.F,
                "vel": Vec2(),
            }
        )
        for state in automata.Q
    }

    # setup raylib
    pr.init_window(SIZE[0], SIZE[1], "AFD")
    pr.set_window_state(pr.ConfigFlags.FLAG_WINDOW_RESIZABLE)
    pr.set_target_fps(FPS)
    pr.set_exit_key(-1)
    font = pr.load_font(font_path)

    global bg_texture, bg_frag_shader
    bg = pr.gen_image_color(SIZE[0], SIZE[1], BACKGROUND_COLOR)
    bg_texture = pr.load_texture_from_image(bg)
    pr.unload_image(bg)

    bg_frag_shader = pr.load_shader("", "./bg.frag")

    name_lens = {
        name: pr.measure_text_ex(font, name, FONT_SIZE, 0).x for name in automata.Q
    }
    max_r = max(name_lens.values()) + 2

    camera2D = pr.Camera2D(
        [pr.get_screen_width() / 2, pr.get_screen_height() / 2], [0, 0], 0, 1
    )


def update(dt: float):
    global \
        dragging_node, \
        feeding, \
        feeding_finished, \
        feeding_idx, \
        feeding_timer, \
        mouse_over_node, \
        mouse_pos, \
        msg, \
        msg_timer, \
        mouse_world_pos

    if feeding:
        if feeding_idx < len(feeding_str):
            feeding_timer += dt

            if feeding_timer >= feeding_delay:
                feeding_timer -= feeding_delay
                _, err = automata.feed_one(feeding_str[feeding_idx])

                # abort and notify
                if err:
                    msg = err
                    msg_timer = 4
                    feeding = False

                feeding_idx += 1
        else:
            feeding_finished = True
            feeding = False

    mouse_pos = pr.get_mouse_position()
    mouse_world_pos = pr.get_screen_to_world_2d(mouse_pos, camera2D)

    if not dragging_node:
        mouse_over_node = None
        for state, node in nodes.items():
            if pr.check_collision_point_circle(
                mouse_world_pos,
                node["pos"].v,
                max_r,
            ):
                mouse_over_node = state
                break


def update_physics(dt: float):
    # apply forces
    applied_edges = set()
    for state, node in nodes.items():
        for input in automata.sigma:
            next_state = automata.delta[state][input]

            # dont apply force to itself
            if state == next_state:
                continue

            if (
                state + "-" + next_state in applied_edges
                or next_state + "-" + state in applied_edges
            ):
                continue

            applied_edges.add(state + "-" + next_state)

            f = Vec2()
            dir = nodes[next_state]["pos"] - node["pos"]
            distance = dir.length()
            edge_len = max(0, distance - max_r * (2 + force_length_start))
            dir = dir.normalize()

            f = f + dir * edge_force * edge_len
            node["vel"] = node["vel"] + f * dt
            nodes[next_state]["vel"] = nodes[next_state]["vel"] - f * dt

    for i in range(automata.n_nodes):
        node = nodes[automata.Q[i]]
        for j in range(i + 1, automata.n_nodes):
            next_node = nodes[automata.Q[j]]
            dir = next_node["pos"] - node["pos"]
            dist = max(max_r * 2, dir.length() - max_r * (2 + force_length_start))
            dir = dir.normalize()
            f = dir * node_repulsion / (dist**2)
            node["vel"] -= f * dt
            next_node["vel"] += f * dt

    for node in nodes.values():
        vel_dir = node["vel"].normalize()
        node["vel"] = node["vel"] - (vel_dir * deccel)
        node["pos"] = node["pos"] + (node["vel"] * dt)


def draw(dt: float):
    pr.clear_background(BACKGROUND_COLOR)

    pr.set_shader_value(
        bg_frag_shader,
        pr.rl_get_location_uniform(bg_frag_shader.id, "resolution"),
        pr.Vector2(pr.get_render_width(), pr.get_render_height()),
        pr.ShaderUniformDataType.SHADER_UNIFORM_VEC2,
    )
    pr.begin_shader_mode(bg_frag_shader)
    pr.draw_texture(bg_texture, 0, 0, pr.WHITE)
    pr.end_shader_mode()

    pr.begin_mode_2d(camera2D)
    # {

    global feeding_str, typing_str, live_mode, msg, msg_timer, help_hint_timer

    next_input = None
    if feeding_idx < len(feeding_str):
        next_input = feeding_str[feeding_idx]

    edge_tags_acc: dict[Vec2, Tuple[list[str], pr.Color]] = {}
    for state, node in nodes.items():
        # draw edges
        self_arrow_drawed = False
        for input in automata.sigma:
            edge_color = pr.GRAY
            if (
                state == automata.current_state
                and feeding_idx < len(feeding_str)
                and not automata.is_valid_input(next_input)
            ):
                edge_color = pr.RED
            next_state = automata.delta[state][input]
            if state == automata.current_state and next_input == input:
                edge_color = pr.GREEN

            if next_state == state:
                center_offset = Vec2()
                for other_state, other_node in nodes.items():
                    if state != other_state:
                        center_offset += node["pos"] - other_node["pos"]

                center_offset = center_offset.normalize() * 1.8 * max_r
                if not self_arrow_drawed:
                    angle = degrees(atan2(center_offset.y, center_offset.x))
                    start = (angle + 225 + PADDING_FROM_NODE) % 360
                    end = start + 270 - 2 * PADDING_FROM_NODE

                    pr.draw_ring(
                        (node["pos"] + center_offset).v,
                        max_r - 2,
                        max_r,
                        start,
                        end,
                        20,
                        edge_color,
                    )
                    end_rad = radians(end)
                    arrow_end = (
                        node["pos"]
                        + center_offset
                        + max_r * Vec2(cos(end_rad), sin(end_rad))
                    )
                    end -= TRIANGLE_SIZE
                    end_rad = radians(end)
                    arrow_end_back = (
                        node["pos"]
                        + center_offset
                        + max_r * Vec2(cos(end_rad), sin(end_rad))
                    )
                    dir = (arrow_end - arrow_end_back).normalize()
                    perp = Vec2(-dir.y, dir.x)
                    pr.draw_triangle(
                        arrow_end.v,
                        (arrow_end_back - perp * TRIANGLE_SIZE * 0.5).v,
                        (arrow_end_back + perp * TRIANGLE_SIZE * 0.5).v,
                        edge_color,
                    )

                tag_pos = (
                    node["pos"]
                    + (2.8 * max_r + TAG_SEPARATION) * center_offset.normalize()
                )

                if not edge_tags_acc.get(tag_pos):
                    edge_tags_acc[tag_pos] = [input], edge_color
                else:
                    edge_tags_acc[tag_pos][0].append(input)

                self_arrow_drawed = True
                continue

            start = node["pos"]
            end = nodes[next_state]["pos"]
            dir = (end - start).normalize()

            padding = max_r + PADDING_FROM_NODE
            # draw edge arrows
            start = start + (dir * padding)
            end = end - (dir * padding)
            control_point = ((end - start) * 1 / 2) + start
            dir_perp = dir.rotate(-pi / 2)
            control_point = control_point + (dir_perp * SEP_FACTOR)

            start = start + (dir_perp * SEP_FACTOR * 0.5)
            end = end + (dir_perp * SEP_FACTOR * 0.5)

            pr.draw_spline_bezier_quadratic(
                [start.v, control_point.v, end.v],
                3,
                THICKNESS,
                edge_color,
            )

            # draw arrows heads
            dir = (end - control_point).normalize()
            end_back = end - (dir * TRIANGLE_SIZE)
            pr.draw_triangle(
                end.v,
                (end_back + (dir_perp * TRIANGLE_SIZE * 0.5)).v,
                (end_back - (dir_perp * TRIANGLE_SIZE * 0.5)).v,
                edge_color,
            )

            control_point += dir_perp * TAG_SEPARATION

            if not edge_tags_acc.get(control_point):
                edge_tags_acc[control_point] = [input], edge_color
            else:
                edge_tags_acc[control_point][0].append(input)

        # draw edge tags (inputs)
        for pos, (tags, color) in edge_tags_acc.items():
            tags_str = ",".join(tags)
            tags_size = Vec2(
                vec=pr.measure_text_ex(font, tags_str, FONT_SIZE * EDGE_TAG_SCALE, 0)
            )
            pr.draw_text_ex(
                font, tags_str, (pos - tags_size / 2).v, tags_size.y, 0, color
            )

        # draw nodes
        active = automata.current_state == state
        node_color = pr.color_brightness(pr.BLUE if active else pr.GRAY, 0.6)
        node_color = pr.GREEN if active and node["is_target"] else node_color

        pr.draw_circle_v(node["pos"].v, max_r, node_color)
        pr.draw_text_ex(
            font,
            state,
            [
                int(node["pos"].x) - name_lens[state] // 2,
                int(node["pos"].y) - FONT_SIZE // 2,
            ],
            FONT_SIZE,
            0,
            pr.BLACK,
        )
        if node["is_target"]:
            pr.draw_ring(node["pos"].v, max_r * 1.2, max_r * 1.3, 0, 360, 0, pr.GRAY)

    # }
    pr.end_mode_2d()

    # gui
    pr.gui_check_box([10, 10, 40, 20], "Live mode", live_mode)

    # msg and input
    if typing_str:
        pr.draw_text_ex(
            font,
            "Enter string: " + feeding_str,
            [FONT_SIZE // 2, pr.get_render_height() - int(FONT_SIZE * 1.5)],
            FONT_SIZE,
            0,
            pr.BLACK,
        )
        msg = None
    elif msg and msg_timer > 0:
        msg_timer -= dt
        pr.draw_text_ex(
            font,
            "Error: " + msg,
            [FONT_SIZE // 2, pr.get_render_height() - int(FONT_SIZE * 1.5)],
            FONT_SIZE,
            0,
            pr.Color(200, 0, 0, int(min(255, 255 * msg_timer))),
        )

    # feeding progress
    if feeding:
        str_cursor_after = pr.measure_text_ex(
            font, feeding_str[: feeding_idx + 1], FONT_SIZE, 0
        ).x
        str_cursor_offset = (
            pr.measure_text_ex(font, feeding_str[:feeding_idx], FONT_SIZE, 0).x
            + str_cursor_after
        ) / 2
        str_width = pr.measure_text_ex(font, feeding_str, FONT_SIZE, 0).x
        str_pos = (pr.get_render_width() - str_width) // 2
        pr.draw_text_ex(
            font,
            feeding_str,
            [str_pos, 20],
            FONT_SIZE,
            0,
            pr.GRAY,
        )
        r = 4
        c = (
            pr.GREEN
            if feeding_idx < len(feeding_str)
            and automata.is_valid_input(feeding_str[feeding_idx])
            else pr.RED
        )
        pr.draw_circle_v([str_pos + str_cursor_offset, 20 + FONT_SIZE + r * 2], r, c)

    elif feeding_finished:
        finished_msg = "Feed finished"
        msg_w = pr.measure_text_ex(font, finished_msg, FONT_SIZE, 0).x
        pr.draw_text_ex(
            font,
            finished_msg,
            [(pr.get_render_width() - msg_w) // 2, 20],
            FONT_SIZE,
            0,
            pr.BLACK,
        )

    if help_hint_timer > 0:
        help_hint_timer -= dt
        hint = "Hold H to show help"
        pr.draw_text_ex(
            font,
            hint,
            [
                (pr.get_render_width() - pr.measure_text_ex(font, hint, FONT_SIZE, 0).x)
                // 2,
                80,
            ],
            FONT_SIZE,
            0,
            pr.color_alpha(pr.BLACK, help_hint_timer),
        )

    if show_help:
        pr.draw_rectangle_rec(
            [0, 0, pr.get_render_width(), pr.get_render_height()],
            pr.Color(0, 0, 0, 128),
        )
        help_size = pr.measure_text_ex(font, help_info, FONT_SIZE, 1)
        pr.draw_text_ex(
            font,
            help_info,
            [
                (pr.get_render_width() - int(help_size.x)) // 2,
                (pr.get_render_height() - int(help_size.y)) // 2,
            ],
            FONT_SIZE,
            0,
            pr.BLACK,
        )


# TODO: move camera with middle mouse
def input():
    global \
        running, \
        typing_str, \
        feeding_str, \
        feeding, \
        feeding_idx, \
        feeding_finished, \
        dragging_node, \
        mouse_over_node, \
        show_help

    if (
        not dragging_node
        and mouse_over_node is not None
        and pr.is_mouse_button_down(pr.MouseButton.MOUSE_BUTTON_LEFT)
    ):
        dragging_node = True

    if dragging_node and mouse_over_node:
        nodes[mouse_over_node]["pos"] = Vec2(
            int(mouse_world_pos.x), int(mouse_world_pos.y)
        )
        if pr.is_mouse_button_up(pr.MouseButton.MOUSE_BUTTON_LEFT):
            dragging_node = False
            mouse_over_node = None

    if not typing_str and pr.is_key_pressed(pr.KeyboardKey.KEY_Q):
        running = False

    if pr.is_key_pressed(pr.KeyboardKey.KEY_ENTER) or pr.is_key_pressed(
        pr.KeyboardKey.KEY_KP_ENTER
    ):
        if not typing_str:
            typing_str = True
            feeding = False
            feeding_idx = 0
            feeding_finished = False
            automata.reset()
        else:
            feeding = True
            feeding_finished = False
            feeding_idx = 0
            typing_str = False

    if pr.is_key_pressed(pr.KeyboardKey.KEY_ESCAPE):
        automata.reset()
        feeding = False
        feeding_finished = False
        feeding_idx = 0
        feeding_str = ""
        if typing_str:
            typing_str = False

    if typing_str:
        key = pr.get_char_pressed()
        while key:
            if 32 <= key <= 126:  # printable chars
                feeding_str += chr(key)
            key = pr.get_char_pressed()
        if pr.is_key_pressed(pr.KeyboardKey.KEY_BACKSPACE):
            if pr.is_key_pressed(pr.KeyboardKey.KEY_LEFT_CONTROL):
                feeding_str = ""
            else:
                feeding_str = feeding_str[:-1]

    if not show_help and not typing_str and pr.is_key_down(pr.KeyboardKey.KEY_H):
        show_help = True
    elif show_help and pr.is_key_up(pr.KeyboardKey.KEY_H):
        show_help = False


# main
if __name__ == "__main__":
    global running
    running = True
    frame_time = 0
    FIXED_DT = 1.0 / FPS

    init()
    while running and not pr.window_should_close():
        dt = pr.get_frame_time()
        frame_time += dt

        while frame_time >= FIXED_DT:
            update_physics(FIXED_DT)
            frame_time -= FIXED_DT

        update(dt)
        pr.begin_drawing()
        draw(dt)
        pr.end_drawing()
        input()

    pr.close_window()
