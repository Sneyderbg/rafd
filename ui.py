from math import pi
from typing import TypedDict
from utils import Vec2

import pyray as pr

import afd1
from afd import AFD


class Node(TypedDict):
    pos: Vec2
    vel: Vec2
    is_target: bool


FONT_SIZE = 24


def init(fps: int):
    global running, camera2D, nodes, automata, n
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

    pr.init_window(800, 600, "AFD")
    pr.set_target_fps(fps)

    camera2D = pr.Camera2D(
        [pr.get_screen_width() / 2, pr.get_screen_height() / 2], [0, 0], 0, 1
    )

    global max_r, name_lens
    name_lens = {name: pr.measure_text(name, FONT_SIZE) for name in automata.Q}
    max_r = max(name_lens.values()) + 2


dragging_node = False
mouse_over_node = None


def update():
    global dragging_node, mouse_over_node

    mouse_pos = pr.get_mouse_position()
    world_pos = pr.get_screen_to_world_2d(mouse_pos, camera2D)

    if not dragging_node:
        mouse_over_node = None
        for state, node in nodes.items():
            if pr.check_collision_point_circle(
                world_pos,
                node["pos"].v,
                max_r,
            ):
                mouse_over_node = state
                break

    if (
        not dragging_node
        and mouse_over_node is not None
        and pr.is_mouse_button_down(pr.MouseButton.MOUSE_BUTTON_LEFT)
    ):
        dragging_node = True

    if dragging_node and mouse_over_node:
        nodes[mouse_over_node]["pos"] = Vec2(int(world_pos.x), int(world_pos.y))
        if pr.is_mouse_button_up(pr.MouseButton.MOUSE_BUTTON_LEFT):
            dragging_node = False
            mouse_over_node = None


def update_physics(dt: float):
    edge_force = 5.0
    node_repulsion = 1e6
    deccel = 2.0
    force_length_start = 4  # from when to apply the respective forces in max_r units

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


def draw():
    pr.begin_mode_2d(camera2D)
    pr.clear_background(pr.RAYWHITE)
    # {

    for state, node in nodes.items():
        # drawing edges
        for input in automata.sigma:
            next_state = automata.delta[state][input]
            if next_state == state:
                # TODO: draw self connection
                continue

            start = node["pos"]
            end = nodes[next_state]["pos"]
            dir = (end - start).normalize()

            # style
            padding = max_r + 10
            sep_factor = 20
            triangle_size = 20
            thickness = 2
            edge_tag_scale = 0.6

            # arrows
            start = start + (dir * padding)
            end = end - (dir * padding)
            control_point = ((end - start) * 1 / 2) + start
            dir_perp = dir.rotate(-pi / 2)
            control_point = control_point + (dir_perp * sep_factor)

            start = start + (dir_perp * sep_factor * 0.5)
            end = end + (dir_perp * sep_factor * 0.5)

            pr.draw_spline_bezier_quadratic(
                [start.v, control_point.v, end.v],
                3,
                thickness,
                pr.GRAY,
            )

            # arrows heads
            dir = (end - control_point).normalize()
            end_back = end - (dir * triangle_size)
            pr.draw_triangle(
                end.v,
                (end_back + (dir_perp * triangle_size * 0.5)).v,
                (end_back - (dir_perp * triangle_size * 0.5)).v,
                pr.GRAY,
            )

            # edge tags (inputs)
            input_w = pr.measure_text(input, int(FONT_SIZE * edge_tag_scale))
            pr.draw_text(
                input,
                int(control_point.x) - input_w // 2,
                int(control_point.y - FONT_SIZE * edge_tag_scale / 2),
                int(FONT_SIZE * edge_tag_scale),
                pr.BLACK,
            )

        # drawing nodes
        active = automata.state == state
        color = pr.color_brightness(pr.BLUE, 0.6) if active else pr.get_color(0x161616)
        color = pr.GREEN if active and node["is_target"] else color

        pr.draw_circle_v(node["pos"].v, max_r, color)
        pr.draw_text(
            state,
            int(node["pos"].x) - name_lens[state] // 2,
            int(node["pos"].y) - FONT_SIZE // 2,
            FONT_SIZE,
            pr.BLACK,
        )
        if node["is_target"]:
            pr.draw_ring(node["pos"].v, max_r * 1.2, max_r * 1.3, 0, 360, 0, pr.GRAY)

    # }
    pr.end_mode_2d()

    # gui
    # TODO: noc, custom? simple?


def input():
    global running
    if pr.is_key_pressed(pr.KeyboardKey.KEY_Q):
        running = False


if __name__ == "__main__":
    global running
    running = True
    frame_time = 0
    FIXED_DT = 1.0 / 100.0

    init(100)
    while running and not pr.window_should_close():
        dt = pr.get_frame_time()
        frame_time += dt

        while frame_time >= FIXED_DT:
            update_physics(FIXED_DT)
            frame_time -= FIXED_DT

        update()
        pr.begin_drawing()
        draw()
        pr.end_drawing()
        input()

    pr.close_window()
