import bpy
from mathutils import Vector

# stuff


#Get active object
activeObj =  bpy.context.active_object
activeName = activeObj.name

if activeObj is None:
    print("No active object. bailing")
    raise ValueError("Error: No active object.")

# Get the active collection
active_collection = bpy.context.view_layer.active_layer_collection.collection 
if active_collection is None:
    print("No active collection. bailing")
    raise ValueError("Error: No active collection.")
    
print("Duplicating " + activeName)

bpy.ops.object.duplicate()

for mod in activeObj.modifiers:
     bpy.ops.object.modifier_apply(modifier=mod.name)

print("Splitting mesh")
bpy.ops.mesh.separate(type='LOOSE')

sel =  bpy.context.selected_objects

# Create a new collection
new_collection = bpy.data.collections.new("Loose Parts: " + activeName)


# Add the new collection as a child of the active collection
active_collection.children.link(new_collection)

#Calculate scale factor for printing. Doesn't support anything but mm and m
scale_factor =  bpy.context.scene.unit_settings.scale_length
if bpy.context.scene.unit_settings.length_unit == 'MILLIMETERS':
    scale_factor = scale_factor * 1000
 
#Iterate over all loose parts
led_id = 0
for part in sel:
    # Move the object to the new collection
    try:
        active_collection.objects.unlink(part)
    except:
        print('no active colletion')
    new_collection.objects.link(part)

    # Get the object's mesh data
    mesh = part.data
    # Convert the local space coordinates to world space coordinates
    world_coords = [part.matrix_world @ v.co for v in mesh.vertices]
    # Sum of world_coords
    sum_coord = Vector((0, 0, 0))
    sum_coord = sum([v for v in world_coords], sum_coord)
    # Calculate the median point
    median_point = scale_factor * sum_coord / len(world_coords)
    print(f"{led_id}, {median_point.x}, {median_point.z}")
    led_id = led_id + 1

    
# Set the object as the active object
#bpy.context.view_layer.objects.active = obj

