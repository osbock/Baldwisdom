inch = 25.4;
battery_type = "18650";

//aaa
    battery_length = 45;
    battery_width = 10.5;
    spring_space = 4;
    contact_width = 4;
    tab_width = 4;

//18650
    battery_length = 65;
    battery_width = 18.2;
    spring_space = 5;
    contact_width = 9.5;
    tab_width = 4;


wall_thick = 1.0;
electrode_thick = .8;

battery_recess_length = battery_length+spring_space;
battery_recess_width = battery_width+1*wall_thick;

module box(numcells){
    difference(){
        cube([(battery_recess_width)*numcells+(numcells+1)*wall_thick,
            battery_recess_length+(4*wall_thick)+2*electrode_thick,
            battery_width+2*wall_thick]);
        for (x = [wall_thick: battery_width+2*wall_thick: ((2*wall_thick+1+battery_width))*numcells]){
            translate([x, wall_thick,wall_thick]){
                electrode_recess();
                translate([battery_recess_width/2-contact_width/2,electrode_thick,2])
                    cube([contact_width,4,battery_width+wall_thick]); //hole for electrode
                translate([battery_recess_width/2-2,-wall_thick,0])
                    cube([tab_width,2,3]); //hole for tab
                translate([0,electrode_thick+wall_thick,0])battery_recess();
                translate([0,electrode_thick+2*wall_thick+battery_recess_length,0])
                    electrode_recess();
                translate([battery_recess_width/2-contact_width/2,wall_thick+battery_recess_length,2])
                    cube([contact_width,2,battery_width+wall_thick]); //hole for electrode
                translate([battery_recess_width/2-tab_width/2,2*wall_thick+battery_recess_length+electrode_thick,0])
                    cube([tab_width,4,3]); //hole for tab
            }
            
        }
    }
}
module electrode_recess(){
            cube([battery_recess_width,
              electrode_thick,
              battery_width+3*wall_thick]);
}
module battery_recess(){
    cube([battery_recess_width,
          battery_recess_length,
          battery_width+3*wall_thick]);
}

box(2);
 