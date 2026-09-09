"""Called by photo_details; all lengths in metres, dimensions provisional.

Facing the teaching wall, final Blender -X (Unreal -X) is visual right.
Resolve this AFTER the room mirror, so the mechanism cannot be reversed by it.
"""
delete_where(lambda o:o.name.startswith(("Chalkboard_","Display_")))
box("PhotoTeaching_WarmBacking",(8.2,.035,H),(0,.021,H/2),"panel_wood")
bw, bh, bt = (P[k] for k in ("TEACHING_BOARD_WIDTH","TEACHING_BOARD_HEIGHT","TEACHING_BOARD_THICKNESS"))
cx, cz, rim = (P[k] for k in ("TEACHING_BOARD_CENTER_X","TEACHING_BOARD_CENTER_Z","TEACHING_BOARD_FRAME"))
fixed_y, moving_y = P["TEACHING_BOARD_FIXED_Y"], P["TEACHING_BOARD_MOVING_Y"]
assert moving_y-bt/2 > fixed_y+bt/2+.01, "Overlapping boards need separate depth tracks"
assert 2*cx >= bw, "Closed board panels must not intersect"
assert P["TEACHING_SCREEN_Y"]+.015 < moving_y-bt/2, "Screen must be behind moving board"
scene = bpy.context.scene
scene["teaching_board_open"] = 0.0
scene.id_properties_ui("teaching_board_open").update(min=0.0,max=1.0,
    description="Facing teaching wall: 0 = closed; 1 = right board stacked over left")
moving_parts = []
for moving in (False, True):
    final_x = -cx if moving else cx
    y = moving_y if moving else fixed_y
    parts = [("Panel",(bw,bt,bh),(0,0,0),"board")]
    for side in (-1,1):
        parts.append((f"Side{side}",(rim,bt+.01,bh),(side*(bw-rim)/2,.006,0),"metal"))
        parts.append((f"Edge{side}",(bw,bt+.01,rim),(0,.006,side*(bh-rim)/2),"metal"))
    for name, size, offset, material in parts:
        ox,oy,oz = offset
        obj = box("PhotoTeaching_"+("Moving" if moving else "Fixed")+name,
            size,((final_x+ox)/MODEL_X_SIGN,y+oy,cz+oz),material,.003)
        if moving:
            obj["runtime_dynamic_teaching"] = True
            driver = obj.driver_add("location",0).driver
            var = driver.variables.new(); var.name = "opening"
            var.targets[0].id_type = "SCENE"; var.targets[0].id = scene
            var.targets[0].data_path = '["teaching_board_open"]'
            driver.expression = f"{final_x+ox} + {2*cx} * opening * opening * (3-2*opening)"
            moving_parts.append({"offset":[ox*100,-oy*100,oz*100],
                "size":[v*100 for v in size],"metal":material=="metal"})
# Only end uprights: no fixed central divider across the exposed display.
for x in (-cx-bw/2-.025,cx+bw/2+.025):
    box("PhotoTeaching_EndFrame",(.035,.24,bh+.07),(x/MODEL_X_SIGN,.23,cz),"metal",.004)
for z in (cz-bh/2-.035,cz+bh/2+.035):
    box("PhotoTeaching_Track",(2*cx+bw+.09,.24,.045),(0,.23,z),"metal",.004)
box("PhotoTeaching_ChalkTray",(2*cx+bw+.07,.28,.03),(0,.24,cz-bh/2-.07),"metal",.004)
sw,sh,sy = (P[k] for k in ("TEACHING_SCREEN_WIDTH","TEACHING_SCREEN_HEIGHT","TEACHING_SCREEN_Y"))
assert sw < bw-2*rim and sh < bh-2*rim, "Closed board must cover the complete display"
box("PhotoTeaching_DisplayHousing",(sw+.07,.055,sh+.05),(-cx/MODEL_X_SIGN,sy-.025,cz),"metal",.008)
face = box("PhotoTeaching_DisplayGlass",(sw,.01,sh),(-cx/MODEL_X_SIGN,sy,cz),"screen",.003)
face["runtime_dynamic_teaching"] = True
scene["runtime_teaching_display"] = json.dumps({
    "Closed": [-cx*100,-moving_y*100,cz*100],
    "Open": [cx*100,-moving_y*100,cz*100],
    "Screen": [-cx*100,-sy*100,cz*100],
    "ScreenSize": [sw*100,1.0,sh*100],
    "TravelSeconds": P["TEACHING_BOARD_TRAVEL_SECONDS"], "parts":moving_parts})

# Exercise the editable Blender driver as well as the runtime's separate tests.
moving_panel = next(o for o in DETAIL.objects if o.name.startswith("PhotoTeaching_MovingPanel"))
for opening in (0.0, .5, 1.0, .3, 0.0):
    scene["teaching_board_open"] = opening
    scene.update_tag()
    bpy.context.view_layer.update()
    expected = -cx + 2*cx*opening*opening*(3-2*opening)
    assert abs(moving_panel.location.x-expected) < 1e-5, "Blender sliding-board driver failed"
print("TEACHING_BOARD_DRIVER_PASS: closed, half, open, reverse, returned")
