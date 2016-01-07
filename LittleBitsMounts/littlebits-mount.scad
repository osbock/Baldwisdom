socket_radius = 3.1; //mm
socket_height = 2.25; //mm
socket_interval = 6.63/2; //mm
base_x = 25;
base_y = 9;
base_z = 2.75;
inch =25.4;

module mounting_plate(x_holes,y_holes)
difference(){
    cube([(2*x_holes*socket_interval)-socket_interval-socket_radius,(2*y_holes*socket_interval)-socket_interval-socket_radius,3]);
    for (x=[0:2*socket_interval:(2*x_holes*socket_interval)-2],y=[0:2*socket_interval:(2*y_holes*socket_interval)-2]){
    translate([x,y,0])cylinder(r=socket_radius,h=5,$fn=20);
    translate([x-2,y-1,-1])cube([8,2,6]);
    }
}


// Height is the y direction
// hence columns is x and rows is y
columns = 20;
rows = 20;
width = rows*(socket_radius+socket_interval);
height = columns*(socket_radius+socket_interval);

union(){
     translate([1,1,2])mounting_plate(rows,columns);
     cube([width, height,2]);
    }