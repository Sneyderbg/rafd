import pyray as pr
import raylib as rl
from afd import AFD
from math import cos, pi, sin


from typing import TypedDict

import afd1


class Node(TypedDict):
    name: str
    pos: pr.Vector2
    vel: pr.Vector2
    is_target: bool


FONT_SIZE = 24


def init():
    global running, camera2D, nodes, edges, automata, n
    automata = AFD(afd1.sigma, afd1.Q, afd1.F, afd1.delta)
    n = automata.n_nodes

    r = 50

    def calc_circ_pos(i, n):
        angle = i * 2 * pi / n + pi / 4
        return pr.Vector2(int(r * -cos(angle)), int(r * -sin(angle)))

    nodes = [
        Node(
            {
                "name": state,
                "pos": pr.Vector2(pr.get_random_value(0, n), pr.get_random_value(0, n)),
                "is_target": state in automata.F,
                "vel": pr.Vector2(),
            }
        )
        for i, state in enumerate(automata.Q)
    ]

    edges = [
        [{input: False for input in automata.sigma} for _ in range(n)] for _ in range(n)
    ]

    for i in range(n):
        for j in range(n):
            for input in automata.sigma:
                has_connection = automata.delta[automata.Q[i]][input] == automata.Q[j]
                edges[i][j][input] = has_connection

    pr.init_window(800, 600, "AFD")
    pr.set_target_fps(100)

    camera2D = pr.Camera2D(
        [pr.get_screen_width() / 2, pr.get_screen_height() / 2], [0, 0], 0, 1
    )

    global max_r, name_lens
    name_lens = list(map(lambda x: pr.measure_text(x, FONT_SIZE), automata.Q))
    max_r = max(name_lens) + 2


dragging_node = False
mouse_over_node_idx = -1


def update():
    global dragging_node, mouse_over_node_idx

    mouse_pos = pr.get_mouse_position()
    world_pos = pr.get_screen_to_world_2d(mouse_pos, camera2D)
    dt = pr.get_frame_time()

    edge_force = 4.0
    node_repulsion = 1e6
    deccel = 2.0
    # apply forces
    for i in range(automata.n_nodes):
        for j in range(i + 1, automata.n_nodes):
            f = pr.Vector2()
            edge = edges[i][j]
            dir = pr.vector2_subtract(nodes[j]["pos"], nodes[i]["pos"])
            distance = pr.vector2_length(dir)
            edge_len = max(0, distance - max_r * 2)
            distance = max(max_r * 2, distance - max_r * 4)
            dir = pr.vector2_normalize(dir)
            if any(edge.values()):
                f = pr.vector2_add(f, pr.vector2_scale(dir, edge_force * edge_len))
            f = pr.vector2_subtract(
                f,
                pr.vector2_scale(dir, node_repulsion / distance**2),
            )
            nodes[i]["vel"] = pr.vector2_add(nodes[i]["vel"], pr.vector2_scale(f, dt))
            nodes[j]["vel"] = pr.vector2_subtract(
                nodes[j]["vel"], pr.vector2_scale(f, dt)
            )

    for node in nodes:
        vel_dir = pr.vector2_normalize(node["vel"])
        node["vel"] = pr.vector2_subtract(
            node["vel"], pr.vector2_scale(vel_dir, deccel)
        )
        node["pos"] = pr.vector2_add(node["pos"], pr.vector2_scale(node["vel"], dt))

    if not dragging_node:
        mouse_over_node_idx = -1
        for i in range(automata.n_nodes):
            if pr.check_collision_point_circle(
                world_pos,
                nodes[i]["pos"],
                max_r,
            ):
                mouse_over_node_idx = i
                break

    if (
        not dragging_node
        and mouse_over_node_idx >= 0
        and pr.is_mouse_button_down(pr.MouseButton.MOUSE_BUTTON_LEFT)
    ):
        dragging_node = True

    if dragging_node:
        nodes[mouse_over_node_idx]["pos"] = pr.Vector2(
            int(world_pos.x), int(world_pos.y)
        )
        if pr.is_mouse_button_up(pr.MouseButton.MOUSE_BUTTON_LEFT):
            dragging_node = False
            mouse_over_node_idx = -1


text = None


def draw():
    pr.begin_mode_2d(camera2D)
    pr.clear_background(pr.RAYWHITE)
    # {

    for i in range(automata.n_nodes):
        node = nodes[i]

        # drawing edges
        for j in range(automata.n_nodes):
            for input in automata.sigma:
                if edges[i][j][input]:
                    start = node["pos"]
                    end = nodes[j]["pos"]
                    dir = pr.vector2_normalize(pr.vector2_subtract(end, start))

                    # style
                    padding = max_r + 10
                    sep_factor = 20
                    triangle_size = 20
                    thickness = 2
                    edge_tag_scale = 0.6

                    # arrows
                    start = pr.vector2_add(start, pr.vector2_scale(dir, padding))
                    end = pr.vector2_subtract(end, pr.vector2_scale(dir, padding))
                    control_point = pr.vector2_add(
                        pr.vector2_scale(pr.vector2_subtract(end, start), 1 / 2), start
                    )
                    dir_perp = pr.vector2_rotate(dir, -pi / 2)
                    control_point = pr.vector2_add(
                        control_point, pr.vector2_scale(dir_perp, sep_factor)
                    )

                    start = pr.vector2_add(
                        start, pr.vector2_scale(dir_perp, sep_factor * 0.5)
                    )
                    end = pr.vector2_add(
                        end, pr.vector2_scale(dir_perp, sep_factor * 0.5)
                    )

                    pr.draw_spline_bezier_quadratic(
                        [start, control_point, end],
                        3,
                        thickness,
                        pr.GRAY,
                    )

                    # arrows heads
                    dir = pr.vector2_normalize(pr.vector2_subtract(end, control_point))
                    end_back = pr.vector2_subtract(
                        end, pr.vector2_scale(dir, triangle_size)
                    )
                    pr.draw_triangle(
                        end,
                        pr.vector2_add(
                            end_back, pr.vector2_scale(dir_perp, triangle_size * 0.5)
                        ),
                        pr.vector2_subtract(
                            end_back, pr.vector2_scale(dir_perp, triangle_size * 0.5)
                        ),
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
        active = automata.state == node["name"]
        color = pr.color_brightness(pr.BLUE, 0.6) if active else pr.get_color(0x161616)
        color = pr.GREEN if active and node["is_target"] else color

        pr.draw_circle_v(node["pos"], max_r, color)
        pr.draw_text(
            node["name"],
            int(node["pos"].x) - name_lens[i] // 2,
            int(node["pos"].y) - FONT_SIZE // 2,
            FONT_SIZE,
            pr.BLACK,
        )
        if node["is_target"]:
            pr.draw_ring(node["pos"], max_r * 1.2, max_r * 1.3, 0, 360, 0, pr.GRAY)

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
    init()
    while running and not pr.window_should_close():
        update()
        pr.begin_drawing()
        draw()
        pr.end_drawing()
        input()

    pr.close_window()
