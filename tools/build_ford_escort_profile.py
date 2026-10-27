"""
Script para generar el perfil fisico JSON completo del Ford Escort RS2000 Mk1 (1971)
para el motor de fisicas Rust de Open Formula.
"""

import json
import os

def build_ford_escort_profile(output_dirs):
    # 1. vehicle.json
    manifest = {
        "coordinate_contract": {
            "forward": "-Z",
            "right": "+X",
            "up": "+Y"
        },
        "domains": {
            "aerodynamics": "aerodynamics.json",
            "brakes": "brakes.json",
            "chassis": "chassis.json",
            "coupled_world": "coupled_world.json",
            "driving_assistance": "driving_assistance.json",
            "fuel": "fuel.json",
            "powertrain": "powertrain.json",
            "steering": "steering.json",
            "suspension": "suspension.json",
            "suspension_filters": "suspension_filters.json",
            "tires": "tires.json",
            "tire_filters": "tire_filters.json",
            "underfloor_contact": "underfloor_contact.json"
        },
        "profile_name": "Ford Escort RS2000 Mk1 1971 Works Rally Spec",
        "schema_version": 1,
        "units": "SI",
        "vehicle_identifier": "ford_escort_rs2000_1971"
    }

    # 2. chassis.json
    chassis = {
        "chassis": {
            "center_of_gravity_height_offset": -0.04,
            "front_weight_distribution": 0.54,
            "inertia_multipliers": {
                "x": 1.43,
                "y": 1.569,
                "z": 0.2955
            },
            "vehicle_mass": 820.0,
            "vehicle_mass_excludes_wheel_assemblies": True,
            "vehicle_name": "Ford Escort RS2000 1971 (Mk1 Works Rally)"
        },
        "geometry": {
            "front_track": 1.28,
            "rear_track": 1.28,
            "wheelbase": 2.41
        },
        "physical_mass_inventory": {
            "base_budget_kilograms": 820.0,
            "schema_version": 2,
            "rim_and_tire_mass_kilograms": [20.0, 20.0, 20.0, 20.0],
            "components": [
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "body panels and rollcage",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.15, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [160.0, 0, 0],
                                [0, 200.0, 0],
                                [0, 0, 75.0]
                            ]
                        },
                        "mass_kilograms": 250.0,
                        "name": "bodywork_and_cage",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Type 49 seam welded bodyshell with safety rollcage"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.05,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 230.0,
                        "rationale": "Works rally shell weight specification",
                        "upper_mass_kilograms": 270.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "engine and auxiliaries",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": -0.85},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [25.0, 0, 0],
                                [0, 18.0, 0],
                                [0, 0, 22.0]
                            ]
                        },
                        "mass_kilograms": 155.0,
                        "name": "engine_pinto_2000",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Ford Pinto 2.0L tuned rally engine"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.04,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 145.0,
                        "rationale": "Works rally engine assembly",
                        "upper_mass_kilograms": 165.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "gearbox and differential",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": -0.1, "z": -0.1},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [10.0, 0, 0],
                                [0, 8.0, 0],
                                [0, 0, 12.0]
                            ]
                        },
                        "mass_kilograms": 75.0,
                        "name": "gearbox_and_differential",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "ZF 5-speed close ratio rally gearbox and prop shaft"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.03,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 70.0,
                        "rationale": "Drivetrain assembly",
                        "upper_mass_kilograms": 80.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "front suspension and steering",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": -0.15, "z": -1.205},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [5.0, 0, 0],
                                [0, 3.0, 0],
                                [0, 0, 6.0]
                            ]
                        },
                        "mass_kilograms": 35.0,
                        "name": "front_suspension_and_steering",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "MacPherson struts, steering rack and anti-roll bar"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.03,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 30.0,
                        "rationale": "Front suspension assembly",
                        "upper_mass_kilograms": 40.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "rear axle and springs",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": -0.15, "z": 1.205},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [8.0, 0, 0],
                                [0, 5.0, 0],
                                [0, 0, 10.0]
                            ]
                        },
                        "mass_kilograms": 55.0,
                        "name": "rear_axle_and_leaf_springs",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Live rear axle, leaf springs and radius arms"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.03,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 50.0,
                        "rationale": "Rear axle assembly",
                        "upper_mass_kilograms": 60.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "brakes and hydraulics",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": -0.10, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [4.0, 0, 0],
                                [0, 3.0, 0],
                                [0, 0, 5.0]
                            ]
                        },
                        "mass_kilograms": 25.0,
                        "name": "brakes_and_hydraulics",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Brake master cylinder, pedal box and hydraulic lines"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.03,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 22.0,
                        "rationale": "Braking equipment",
                        "upper_mass_kilograms": 28.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "cooling and radiators",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": -1.4},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [3.0, 0, 0],
                                [0, 2.0, 0],
                                [0, 0, 3.0]
                            ]
                        },
                        "mass_kilograms": 20.0,
                        "name": "radiator_and_fluids",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Water radiator, oil cooler and fluids"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.03,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 18.0,
                        "rationale": "Cooling package",
                        "upper_mass_kilograms": 22.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "electrical and battery",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": -0.2, "y": 0.0, "z": 0.8},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [2.0, 0, 0],
                                [0, 2.0, 0],
                                [0, 0, 2.0]
                            ]
                        },
                        "mass_kilograms": 15.0,
                        "name": "electrical_and_battery",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Rear-mounted racing battery and wiring harness"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.03,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 13.0,
                        "rationale": "Electrical system",
                        "upper_mass_kilograms": 17.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "fuel tank empty",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 1.4},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [2.0, 0, 0],
                                [0, 2.0, 0],
                                [0, 0, 2.0]
                            ]
                        },
                        "mass_kilograms": 10.0,
                        "name": "fuel_tank_empty",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Aluminium rally fuel cell"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.03,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 9.0,
                        "rationale": "Fuel tank weight",
                        "upper_mass_kilograms": 11.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "driver and co-driver",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.1},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [18.0, 0, 0],
                                [0, 16.0, 0],
                                [0, 0, 10.0]
                            ]
                        },
                        "mass_kilograms": 150.0,
                        "name": "crew_driver_codriver",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Driver (75 kg) and Co-driver (75 kg) with bucket seats"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.05,
                        "inertia_relative_uncertainty": 0.1,
                        "lower_mass_kilograms": 140.0,
                        "rationale": "Crew members",
                        "upper_mass_kilograms": 160.0
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "rally equipment and sumpguard",
                    "properties": {
                        "attachment": {"kind": "body"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": -0.15, "z": 0.5},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [5.0, 0, 0],
                                [0, 4.0, 0],
                                [0, 0, 5.0]
                            ]
                        },
                        "mass_kilograms": 22.0,
                        "name": "rally_sumpguard_and_tools",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Duralumin sump guard, fire system and tools"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.05,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 20.0,
                        "rationale": "Protection equipment",
                        "upper_mass_kilograms": 25.0
                    }
                },
                # Upright assemblies
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "front left upright",
                    "properties": {
                        "attachment": {"kind": "upright", "wheel": "FrontLeft"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.03, 0, 0],
                                [0, 0.02, 0],
                                [0, 0, 0.02]
                            ]
                        },
                        "mass_kilograms": 2.0,
                        "name": "front_left_upright",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Ford Escort front hub upright"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 1.8,
                        "rationale": "Hub assembly",
                        "upper_mass_kilograms": 2.2
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "front right upright",
                    "properties": {
                        "attachment": {"kind": "upright", "wheel": "FrontRight"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.03, 0, 0],
                                [0, 0.02, 0],
                                [0, 0, 0.02]
                            ]
                        },
                        "mass_kilograms": 2.0,
                        "name": "front_right_upright",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Ford Escort front hub upright"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 1.8,
                        "rationale": "Hub assembly",
                        "upper_mass_kilograms": 2.2
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "rear left upright",
                    "properties": {
                        "attachment": {"kind": "upright", "wheel": "RearLeft"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.03, 0, 0],
                                [0, 0.02, 0],
                                [0, 0, 0.02]
                            ]
                        },
                        "mass_kilograms": 2.0,
                        "name": "rear_left_upright",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Ford Escort rear hub upright"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 1.8,
                        "rationale": "Hub assembly",
                        "upper_mass_kilograms": 2.2
                    }
                },
                {
                    "budget": "vehicle_and_driver",
                    "component_perimeter": "rear right upright",
                    "properties": {
                        "attachment": {"kind": "upright", "wheel": "RearRight"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.03, 0, 0],
                                [0, 0.02, 0],
                                [0, 0, 0.02]
                            ]
                        },
                        "mass_kilograms": 2.0,
                        "name": "rear_right_upright",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Ford Escort rear hub upright"
                        }
                    },
                    "rotating_wheel": None,
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.2,
                        "lower_mass_kilograms": 1.8,
                        "rationale": "Hub assembly",
                        "upper_mass_kilograms": 2.2
                    }
                },
                # Wheel rotors
                {
                    "budget": "rim_and_tire",
                    "component_perimeter": "front left wheel assembly",
                    "properties": {
                        "attachment": {"kind": "wheel_rotor", "wheel": "FrontLeft"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.80, 0, 0],
                                [0, 0.50, 0],
                                [0, 0, 0.50]
                            ]
                        },
                        "mass_kilograms": 20.0,
                        "name": "wheel_frontleft",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Minilite rally wheel with 175/70 R13 gravel tyre"
                        }
                    },
                    "rotating_wheel": "FrontLeft",
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.1,
                        "lower_mass_kilograms": 19.0,
                        "rationale": "Wheel assembly",
                        "upper_mass_kilograms": 21.0
                    }
                },
                {
                    "budget": "rim_and_tire",
                    "component_perimeter": "front right wheel assembly",
                    "properties": {
                        "attachment": {"kind": "wheel_rotor", "wheel": "FrontRight"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.80, 0, 0],
                                [0, 0.50, 0],
                                [0, 0, 0.50]
                            ]
                        },
                        "mass_kilograms": 20.0,
                        "name": "wheel_frontright",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Minilite rally wheel with 175/70 R13 gravel tyre"
                        }
                    },
                    "rotating_wheel": "FrontRight",
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.1,
                        "lower_mass_kilograms": 19.0,
                        "rationale": "Wheel assembly",
                        "upper_mass_kilograms": 21.0
                    }
                },
                {
                    "budget": "rim_and_tire",
                    "component_perimeter": "rear left wheel assembly",
                    "properties": {
                        "attachment": {"kind": "wheel_rotor", "wheel": "RearLeft"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.80, 0, 0],
                                [0, 0.50, 0],
                                [0, 0, 0.50]
                            ]
                        },
                        "mass_kilograms": 20.0,
                        "name": "wheel_rearleft",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Minilite rally wheel with 175/70 R13 gravel tyre"
                        }
                    },
                    "rotating_wheel": "RearLeft",
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.1,
                        "lower_mass_kilograms": 19.0,
                        "rationale": "Wheel assembly",
                        "upper_mass_kilograms": 21.0
                    }
                },
                {
                    "budget": "rim_and_tire",
                    "component_perimeter": "rear right wheel assembly",
                    "properties": {
                        "attachment": {"kind": "wheel_rotor", "wheel": "RearRight"},
                        "center_of_mass_in_attachment_metres": {"x": 0.0, "y": 0.0, "z": 0.0},
                        "inertia_about_center_of_mass": {
                            "entries_kilogram_square_metres": [
                                [0.80, 0, 0],
                                [0, 0.50, 0],
                                [0, 0, 0.50]
                            ]
                        },
                        "mass_kilograms": 20.0,
                        "name": "wheel_rearright",
                        "provenance": {
                            "center_of_mass_origin": "estimated",
                            "inertia_origin": "estimated",
                            "mass_origin": "estimated",
                            "source": "Minilite rally wheel with 175/70 R13 gravel tyre"
                        }
                    },
                    "rotating_wheel": "RearRight",
                    "uncertainty": {
                        "center_uncertainty_metres": 0.02,
                        "inertia_relative_uncertainty": 0.1,
                        "lower_mass_kilograms": 19.0,
                        "rationale": "Wheel assembly",
                        "upper_mass_kilograms": 21.0
                    }
                }
            ]
        }
    }

    # 3. powertrain.json
    powertrain = {
        "powertrain": {
            "automatic_shift": {
                "downshift_coast_normalized_rpm": 0.28,
                "downshift_normalized_rpm_full_throttle": 0.55,
                "downshift_normalized_rpm_low_throttle": 0.38,
                "downshift_target_rpm_margin_high": 0.85,
                "downshift_throttle_aggression_factor": 0.4,
                "kickdown_delay_factor": 0.7,
                "parked_resume_speed_ms": 1.0,
                "reverse_max_speed_ms": 10.0,
                "road_spin_weight": 0.4,
                "upshift_coast_normalized_rpm": 0.38,
                "upshift_normalized_rpm_full_throttle": 0.94,
                "upshift_normalized_rpm_low_throttle": 0.65,
                "upshift_target_rpm_margin_high": 0.95,
                "wheel_road_spin_blend_threshold_rads": 8.0,
                "wheel_spin_weight": 0.6
            },
            "automatic_transmission": False,
            "clutch_out_rpm_offset": 600.0,
            "constant_brake_ratio": 0.01,
            "differential": {
                "clutch_friction_coefficient": 0.12,
                "clutches": 4.0,
                "coast_ramp_angle_deg": 60.0,
                "power_ramp_angle_deg": 45.0,
                "preload_nm": 75.0,
                "slip_transition_threshold_rad_s": 4.0
            },
            "downshift_blip_torque_fraction": 0.8,
            "downshift_clutch_reengagement_seconds": 0.12,
            "downshift_maximum_target_revolutions_per_minute_fraction": 0.92,
            "downshift_minimum_interval_seconds": 0.25,
            "downshift_rev_matching_enabled": True,
            "final_drive": 4.11,
            "front_torque_split": 0.0,
            "gear_inertia": 0.025,
            "gear_ratios": [
                2.30,
                1.67,
                1.22,
                1.00,
                0.87
            ],
            "handbrake_torque_fraction": 1.0,
            "idle_disengagement_hysteresis_rpm": 150.0,
            "idle_rpm": 1100.0,
            "max_clutch_torque_ratio": 1.35,
            "max_rpm": 7500.0,
            "max_torque": 205.0,
            "motor_moment": 0.18,
            "reverse_ratio": 3.0,
            "shift_time": 0.15,
            "thermal": {
                "engine_block_heat_capacity_joules_per_kelvin": 65000.0,
                "engine_block_heat_fraction": 0.12,
                "engine_to_oil_conductance_watts_per_kelvin": 380.0,
                "engine_to_water_conductance_watts_per_kelvin": 850.0,
                "exhaust_heat_fraction": 0.60,
                "heat_generation_per_mechanical_power": 1.5,
                "idle_heat_generation_watts": 12000.0,
                "initial_engine_block_temperature_celsius": 85.0,
                "initial_oil_temperature_celsius": 85.0,
                "initial_water_temperature_celsius": 82.0,
                "minimum_hot_engine_torque_fraction": 0.85,
                "oil_cooling_duct": {
                    "air_specific_heat_joules_per_kilogram_kelvin": 1005.0,
                    "area_response_exponent": 1.0,
                    "base_cooling_conductance_watts_per_kelvin": 40.0,
                    "discharge_coefficient": 0.70,
                    "drag_coefficient": 0.40,
                    "heat_exchanger_conductance_watts_per_kelvin": 2000.0,
                    "maximum_inlet_area_square_meters": 0.06,
                    "opening": 0.50,
                    "pressure_recovery": 0.70
                },
                "oil_critical_temperature_celsius": 135.0,
                "oil_heat_capacity_joules_per_kelvin": 15000.0,
                "oil_heat_fraction": 0.08,
                "oil_hot_derating_temperature_celsius": 120.0,
                "oil_minimum_cold_engine_torque_fraction": 0.88,
                "oil_optimal_maximum_temperature_celsius": 110.0,
                "oil_optimal_minimum_temperature_celsius": 80.0,
                "water_cooling_duct": {
                    "air_specific_heat_joules_per_kilogram_kelvin": 1005.0,
                    "area_response_exponent": 1.0,
                    "base_cooling_conductance_watts_per_kelvin": 70.0,
                    "discharge_coefficient": 0.72,
                    "drag_coefficient": 0.50,
                    "heat_exchanger_conductance_watts_per_kelvin": 4000.0,
                    "maximum_inlet_area_square_meters": 0.20,
                    "opening": 0.75,
                    "pressure_recovery": 0.75
                },
                "water_critical_temperature_celsius": 115.0,
                "water_heat_capacity_joules_per_kelvin": 28000.0,
                "water_heat_fraction": 0.20,
                "water_hot_derating_temperature_celsius": 105.0,
                "water_minimum_cold_engine_torque_fraction": 0.95,
                "water_optimal_maximum_temperature_celsius": 95.0,
                "water_optimal_minimum_temperature_celsius": 75.0
            },
            "torque_attack_rate_nm_s": 850.0,
            "torque_curve": [
                [0.0, 0.35],
                [0.15, 0.45],
                [0.25, 0.60],
                [0.35, 0.75],
                [0.45, 0.88],
                [0.55, 0.96],
                [0.64, 1.00],
                [0.75, 0.98],
                [0.85, 0.92],
                [0.93, 0.84],
                [1.00, 0.72]
            ],
            "variable_drag_ratio": 0.05
        }
    }

    # 4. suspension.json
    suspension = {
        "suspension": {
            "use_authored_wheel_centers": True,
            "virtual_unsprung_mass_multiplier": 1.15,
            "contact_ray_count": 1,
            "tri_ray_spacing_ratio": 0.35,
            "model": "geometric",
            "model_version": 1,
            "front": {
                "arb_ratio": 0.35,
                "bump_damp_multiplier": 1.0,
                "bump_stop_multiplier": 1.0,
                "camber": -0.017453,
                "camber_gain_rad_per_m": -0.015,
                "damping_ratio": 0.65,
                "rebound_damp_multiplier": 1.20,
                "resting_ratio": 0.35,
                "spring_length": 0.32,
                "toe": 0.001745,
                "toe_gain_rad_per_m": 0.01
            },
            "rear": {
                "arb_ratio": 0.15,
                "bump_damp_multiplier": 1.0,
                "bump_stop_multiplier": 1.0,
                "camber": 0.0,
                "camber_gain_rad_per_m": 0.0,
                "damping_ratio": 0.65,
                "rebound_damp_multiplier": 1.20,
                "resting_ratio": 0.35,
                "spring_length": 0.30,
                "toe": 0.0,
                "toe_gain_rad_per_m": 0.0
            },
            "geometry_physical": {
                "version": 1,
                "corners": {
                    "FL": {
                        "damper": {
                            "chassis": [-0.084993, -0.296701, -0.97]
                        },
                        "hub_center": [-0.64, -0.226701, -1.205],
                        "lower_wishbone": {
                            "inner_front": [-0.14044, -0.258956, -1.250731],
                            "inner_rear": [-0.162507, -0.257959, -0.793059],
                            "outer": [-0.497918, -0.258401, -1.208898]
                        },
                        "provenance": {
                            "origin": "reconstructed",
                            "note": "Transformed to Escort wheelbase 2.41m and track 1.28m: target hub z=-1.205"
                        },
                        "rocker": {
                            "axis": [1.0, 0.0, 0.0],
                            "damper_arm": [-0.084993, -0.096266, -1.03],
                            "pivot": [-0.084993, -0.096266, -1.176208],
                            "pushrod_arm": [-0.084993, -0.096266, -1.336208]
                        },
                        "rod": {
                            "attachment": "lower",
                            "outer": [-0.467242, -0.245975, -1.168929],
                            "type": "pushrod"
                        },
                        "trackrod": {
                            "inner": [-0.139086, -0.259653, -1.336327],
                            "outer": [-0.494653, -0.260107, -1.316078]
                        },
                        "upper_wishbone": {
                            "inner_front": [-0.14044, -0.090605, -1.250731],
                            "inner_rear": [-0.162507, -0.089608, -0.926632],
                            "outer": [-0.497918, -0.090051, -1.208898]
                        }
                    },
                    "FR": {
                        "mirror_of": "FL"
                    },
                    "RL": {
                        "damper": {
                            "chassis": [-0.089386, -0.116634, 1.07]
                        },
                        "driveshaft": {
                            "inner": [-0.074741, -0.220012, 1.217802],
                            "outer": [-0.64, -0.216634, 1.205]
                        },
                        "hub_center": [-0.64, -0.216634, 1.205],
                        "lower_wishbone": {
                            "inner_front": [-0.096376, -0.296011, 0.878268],
                            "inner_rear": [-0.065836, -0.33699, 1.354112],
                            "outer": [-0.451569, -0.276237, 1.204073]
                        },
                        "provenance": {
                            "origin": "reconstructed",
                            "note": "Transformed to Escort wheelbase 2.41m and track 1.28m: target hub z=1.205"
                        },
                        "rocker": {
                            "axis": [1.0, 0.0, 0.0],
                            "damper_arm": [-0.178771, -0.361634, 1.0442],
                            "pivot": [-0.178771, -0.361634, 1.12],
                            "pushrod_arm": [-0.178771, -0.361634, 0.98]
                        },
                        "rod": {
                            "attachment": "upper",
                            "outer": [-0.42905, -0.106634, 1.16],
                            "type": "pullrod"
                        },
                        "trackrod": {
                            "inner": [-0.075978, -0.284223, 1.34],
                            "outer": [-0.451569, -0.231634, 1.28]
                        },
                        "upper_wishbone": {
                            "inner_front": [-0.088078, -0.16134, 0.876033],
                            "inner_rear": [-0.065836, -0.165389, 1.354112],
                            "outer": [-0.451569, -0.100587, 1.204073]
                        }
                    },
                    "RR": {
                        "mirror_of": "RL"
                    }
                },
                "front": {
                    "spring_rate_N_per_m": 45000.0,
                    "spring_free_length_m": 0.35,
                    "spring_installed_length_m": 0.30,
                    "damper_bump_Ns_per_m": 2500.0,
                    "damper_rebound_Ns_per_m": 3000.0,
                    "damper_fast_factor": 0.5,
                    "damper_knee_m_per_s": 0.15,
                    "damper_knee_blend_m_per_s": 0.02,
                    "damper_min_m": 0.15,
                    "damper_max_m": 0.45,
                    "wheel_bump_m": 0.12,
                    "wheel_droop_m": 0.10,
                    "bump_stop_travel_m": 0.02,
                    "bump_stop_mult": 1.0,
                    "bump_stop_law": "cubic",
                    "bump_stop_damper_Ns_per_m": 600.0,
                    "hard_stop_overtravel_m": 0.005,
                    "hard_stop_compliance_m": 0.001,
                    "hard_stop_stiffness_N_per_m": 8000000.0,
                    "hard_stop_damper_Ns_per_m": 2500.0,
                    "arb_damper_Ns_per_m": 120.0
                },
                "front_arb": {
                    "mode": "legacy_ratio",
                    "ratio": 0.35
                },
                "rear": {
                    "spring_rate_N_per_m": 40000.0,
                    "spring_free_length_m": 0.35,
                    "spring_installed_length_m": 0.30,
                    "damper_bump_Ns_per_m": 2500.0,
                    "damper_rebound_Ns_per_m": 2000.0,
                    "damper_fast_factor": 0.5,
                    "damper_knee_m_per_s": 0.15,
                    "damper_knee_blend_m_per_s": 0.02,
                    "damper_min_m": 0.15,
                    "damper_max_m": 0.45,
                    "wheel_bump_m": 0.12,
                    "wheel_droop_m": 0.10,
                    "bump_stop_travel_m": 0.02,
                    "bump_stop_mult": 1.0,
                    "bump_stop_law": "cubic",
                    "bump_stop_damper_Ns_per_m": 600.0,
                    "hard_stop_overtravel_m": 0.005,
                    "hard_stop_compliance_m": 0.001,
                    "hard_stop_stiffness_N_per_m": 8000000.0,
                    "hard_stop_damper_Ns_per_m": 2500.0,
                    "arb_damper_Ns_per_m": 80.0
                },
                "rear_arb": {
                    "mode": "legacy_ratio",
                    "ratio": 0.15
                }
            }
        }
    }

    # 5. tires.json
    tires = {
        "tires": {
            "radial_damping_ratio": 0.40,
            "airborne_spin_decay_torque": 1.5,
            "contact_patch": 0.20,
            "front": {
                "airborne_spin_decay_torque": 1.5,
                "contact_patch": 0.20,
                "radius": 0.28,
                "wheel_mass": 20.0,
                "width": 0.20
            },
            "rear": {
                "airborne_spin_decay_torque": 1.5,
                "contact_patch": 0.20,
                "radius": 0.28,
                "wheel_mass": 20.0,
                "width": 0.20
            },
            "force_model": {
                "front": {
                    "budget_blend_width": 0.15,
                    "combined_lateral_peak_migration": 0.30,
                    "combined_longitudinal_peak_migration": 0.25,
                    "contact_ramp_tau_s": 0.05,
                    "falloff_sharpness": 1.8,
                    "lateral_peak_slip_angle_rad": 0.14,
                    "lateral_slide_mu_ratio": 0.90,
                    "longitudinal_peak_slip_ratio": 0.15,
                    "longitudinal_slide_mu_ratio": 0.88,
                    "min_relaxation_tau_s": 0.010,
                    "relaxation_recovery_scale": 1.2,
                    "slip_loss_tau_s": 0.06,
                    "slip_recovery_tau_s": 0.20
                },
                "rear": {
                    "budget_blend_width": 0.15,
                    "combined_lateral_peak_migration": 0.30,
                    "combined_longitudinal_peak_migration": 0.25,
                    "contact_ramp_tau_s": 0.05,
                    "falloff_sharpness": 1.8,
                    "lateral_peak_slip_angle_rad": 0.14,
                    "lateral_slide_mu_ratio": 0.90,
                    "longitudinal_peak_slip_ratio": 0.15,
                    "longitudinal_slide_mu_ratio": 0.88,
                    "min_relaxation_tau_s": 0.010,
                    "relaxation_recovery_scale": 1.2,
                    "slip_loss_tau_s": 0.06,
                    "slip_recovery_tau_s": 0.20
                }
            },
            "pressure": {
                "atmospheric_pressure_kpa": 101.325,
                "cold_kpa_gauge": {
                    "FL": 180.0,
                    "FR": 180.0,
                    "RL": 180.0,
                    "RR": 180.0
                },
                "reference_hot_kpa_gauge": {
                    "FL": 210.0,
                    "FR": 210.0,
                    "RL": 210.0,
                    "RR": 210.0
                },
                "reference_temperature_c": 25.0,
                "units": "kPa_gauge"
            },
            "pressure_mechanics": {
                "compliance_sensitivity": 0.60,
                "response_sensitivity": -0.15,
                "rolling_resistance_sensitivity": -0.30
            },
            "surface_transition_tau_s": 0.08,
            "surfaces": {
                "Road": {
                    "friction": 1.20,
                    "lateral_grip_assist": 0.0,
                    "longitudinal_grip_ratio": 0.50,
                    "rolling_resistance": 1.0,
                    "stiffness": 5.0
                },
                "Gravel": {
                    "friction": 0.95,
                    "lateral_grip_assist": 0.0,
                    "longitudinal_grip_ratio": 0.45,
                    "rolling_resistance": 2.2,
                    "stiffness": 0.8
                },
                "Dirt": {
                    "friction": 1.00,
                    "lateral_grip_assist": 0.0,
                    "longitudinal_grip_ratio": 0.45,
                    "rolling_resistance": 2.0,
                    "stiffness": 0.8
                },
                "Grass": {
                    "friction": 0.65,
                    "lateral_grip_assist": 0.0,
                    "longitudinal_grip_ratio": 0.35,
                    "rolling_resistance": 3.5,
                    "stiffness": 0.4
                },
                "Curb": {
                    "friction": 1.30,
                    "lateral_grip_assist": 0.0,
                    "longitudinal_grip_ratio": 0.55,
                    "rolling_resistance": 1.2,
                    "stiffness": 5.0
                }
            }
        }
    }

    # 6. brakes.json
    brakes = {
        "brakes": {
            "abs_pulse_time": 0.03,
            "abs_spin_diff_threshold": 10.0,
            "enable_abs": False,
            "front_brake_bias": 0.62,
            "max_brake_torque": 2400.0,
            "wheel_lock_blend_spin_rad_s": 6.0,
            "wheel_lock_stick_tau_s": 0.03,
            "thermal": {
                "braking_heat_fraction": 0.95,
                "cold_efficiency": 0.85,
                "critical_temperature_c": 750.0,
                "fade_start_temperature_c": 550.0,
                "optimal_max_temperature_c": 450.0,
                "optimal_min_temperature_c": 150.0,
                "initial_temperature_c": 25.0,
                "minimum_fade_efficiency": 0.50,
                "front": {
                    "cooling_profile": "open_wheel_external_faces",
                    "installation_airflow_scale": 0.60,
                    "model": "scaled_two_node_v1",
                    "rim_base_air_w_k": 8.0,
                    "rim_heat_capacity_j_k": 3200.0,
                    "rim_to_tire_carcass_w_k": 35.0,
                    "rim_to_tire_gas_w_k": 25.0,
                    "rotor_inner_diameter_m": 0.14,
                    "rotor_mass_kg": 4.5,
                    "rotor_material": "cast_iron",
                    "rotor_outer_diameter_m": 0.245,
                    "rotor_radiation_ambient_view_factor": 0.4,
                    "rotor_radiation_emissivity": 0.7,
                    "rotor_to_rim_w_k": 12.0,
                    "rotor_ventilation": "solid",
                    "thermal_mass_scale": 1.0
                },
                "front_duct": {
                    "air_specific_heat_j_kg_k": 1005.0,
                    "area_response_exponent": 1.0,
                    "cooling_flow_exponent": 0.75,
                    "cooling_reference_speed_ms": 35.0,
                    "discharge_coefficient": 0.65,
                    "drag_coefficient": 0.15,
                    "heat_exchanger_ua_w_k": 250.0,
                    "max_inlet_area_m2_per_wheel": 0.008,
                    "minimum_cooling_flow_ratio": 0.15,
                    "opening": 0.40,
                    "pressure_recovery": 0.60,
                    "rotor_cooling_fraction": 0.85
                },
                "rear": {
                    "cooling_profile": "open_wheel_external_faces",
                    "installation_airflow_scale": 0.50,
                    "model": "scaled_two_node_v1",
                    "rim_base_air_w_k": 7.0,
                    "rim_heat_capacity_j_k": 3500.0,
                    "rim_to_tire_carcass_w_k": 25.0,
                    "rim_to_tire_gas_w_k": 18.0,
                    "rotor_inner_diameter_m": 0.12,
                    "rotor_mass_kg": 5.0,
                    "rotor_material": "cast_iron",
                    "rotor_outer_diameter_m": 0.203,
                    "rotor_radiation_ambient_view_factor": 0.3,
                    "rotor_radiation_emissivity": 0.7,
                    "rotor_to_rim_w_k": 10.0,
                    "rotor_ventilation": "solid",
                    "thermal_mass_scale": 1.0
                },
                "rear_duct": {
                    "air_specific_heat_j_kg_k": 1005.0,
                    "area_response_exponent": 1.0,
                    "cooling_flow_exponent": 0.75,
                    "cooling_reference_speed_ms": 35.0,
                    "discharge_coefficient": 0.65,
                    "drag_coefficient": 0.10,
                    "heat_exchanger_ua_w_k": 200.0,
                    "max_inlet_area_m2_per_wheel": 0.006,
                    "minimum_cooling_flow_ratio": 0.15,
                    "opening": 0.30,
                    "pressure_recovery": 0.60,
                    "rotor_cooling_fraction": 0.85
                }
            }
        }
    }

    # 7. aerodynamics.json
    aerodynamics = {
        "aero": {
            "air_density": 1.225,
            "blend_full_speed_mps": 35.0,
            "blend_min_speed_mps": 5.0,
            "drag_coefficient": 0.42,
            "frontal_area": 1.85,
            "lag_tau_s": 0.04,
            "model": {
                "body": {
                    "drag_coefficient_scope": "whole_vehicle_reference",
                    "wheel_reference_drag_area_m2": 0.08
                },
                "front_wing": {
                    "area_m2": 0.05,
                    "incidence_deg": 4.0,
                    "min_angle_deg": 0.0,
                    "max_angle_deg": 10.0,
                    "polar": [
                        {"angle_deg": 0.0, "cd": 0.01, "cl": 0.01},
                        {"angle_deg": 5.0, "cd": 0.02, "cl": 0.03},
                        {"angle_deg": 10.0, "cd": 0.04, "cl": 0.05}
                    ],
                    "position_m": [0.0, -0.1, -1.8],
                    "yaw_decay_exponent": 1.0
                },
                "rear_wing": {
                    "area_m2": 0.05,
                    "incidence_deg": 4.0,
                    "min_angle_deg": 0.0,
                    "max_angle_deg": 10.0,
                    "polar": [
                        {"angle_deg": 0.0, "cd": 0.01, "cl": 0.01},
                        {"angle_deg": 5.0, "cd": 0.02, "cl": 0.03},
                        {"angle_deg": 10.0, "cd": 0.04, "cl": 0.05}
                    ],
                    "position_m": [0.0, 0.2, 1.8],
                    "yaw_decay_exponent": 1.0
                },
                "limits": {
                    "hard_max_downforce_n": 500.0,
                    "hard_max_load_ratio": 1.05,
                    "profile": "saloon_rally",
                    "soft_max_load_ratio": 1.02
                },
                "underfloor": {
                    "bottoming_factor": 0.1,
                    "choke_height_m": 0.03,
                    "floor_clearance_front_m": 0.15,
                    "floor_clearance_rear_m": 0.18,
                    "high_height_m": 0.35,
                    "induced_drag_ratio": 0.05,
                    "lift_area_m2": 0.1,
                    "optimal_height_m": 0.16,
                    "optimal_rake_deg": 0.5,
                    "position_m": [0.0, -0.15, 0.0],
                    "rake_window_deg": 5.0,
                    "seal_asymmetry_m": 0.1,
                    "seal_roll_rad": 0.2,
                    "stall_attack_tau_s": 0.08,
                    "stall_recovery_tau_s": 0.12,
                    "throat_probe_offset_m": 0.0,
                    "yaw_decay_exponent": 1.0
                },
                "wheels": {
                    "front_drag_coefficient": 0.04,
                    "rear_drag_coefficient": 0.04
                }
            }
        }
    }

    # 8. steering.json
    steering = {
        "steering": {
            "ackermann": 0.18,
            "countersteer_assist": 0.0,
            "front_steering_ratio": 1.0,
            "max_steering_angle": 0.5585,
            "rear_steering_ratio": 0.0,
            "steering_exponent": 1.1,
            "steering_slip_assist": 0.0,
            "steering_speed": 5.0,
            "steering_speed_decay": 0.05
        }
    }

    # 9. fuel.json
    fuel = {
        "fuel": {
            "brake_specific_consumption_kg_per_kwh": 0.35,
            "capacity_kg": 45.0,
            "estimated_lap_consumption_kg": 2.5,
            "idle_consumption_kg_per_hour": 3.0,
            "initial_kg": 25.0,
            "reference_lap_time_s": 180.0,
            "density_kg_per_litre": 0.74,
            "tank_position_local_m": {
                "x": 0.0,
                "y": 0.0,
                "z": 1.4
            }
        }
    }

    # 10. underfloor_contact.json
    underfloor_contact = {
        "underfloor_contact": {
            "activation_clearance_m": 0.05,
            "approach_clearance_m": 0.08,
            "clearance_filter_seconds": 0.015,
            "contact_hold_seconds": 0.06,
            "damping_n_s_m": 3500.0,
            "enabled": True,
            "linear_rate_n_m": 450000.0,
            "max_force_per_probe_n": 15000.0,
            "normal_min_y": 0.50,
            "progressive_rate_n_m2": 6000000.0,
            "release_clearance_m": 0.12,
            "rigid_contact_spring_scale": 0.30,
            "rigid_hold_seconds": 0.08,
            "rigid_maximum_local_height_metres": -0.05,
            "rigid_minimum_impulse_newton_seconds": 12.0
        }
    }

    # 11. coupled_world.json
    coupled_world = {
        "coupled_world": {
            "aerodynamic_probe_length_metres": 1,
            "aerodynamic_probe_positions_local_metres": [
                {"x": -0.4, "y": -0.15, "z": -1.0},
                {"x": 0.4, "y": -0.15, "z": -1.0},
                {"x": 0.0, "y": -0.15, "z": 0.0},
                {"x": -0.4, "y": -0.15, "z": 1.0},
                {"x": 0.4, "y": -0.15, "z": 1.0}
            ],
            "body_collision_boxes": [
                {
                    "identifier": "cabin_engine",
                    "center_local_metres": {"x": 0.0, "y": 0.10, "z": 0.0},
                    "half_extents_metres": {"x": 0.80, "y": 0.45, "z": 1.95}
                },
                {
                    "identifier": "sump_guard",
                    "center_local_metres": {"x": 0.0, "y": -0.15, "z": -0.9},
                    "half_extents_metres": {"x": 0.55, "y": 0.05, "z": 0.5}
                }
            ],
            "contacts": {
                "impact_friction_coefficient": 0.55,
                "maximum_events_per_step": 128,
                "maximum_iterations": 100,
                "maximum_position_correction_metres": 0.002,
                "maximum_supported_angular_speed_radians_per_second": 12,
                "maximum_supported_speed_metres_per_second": 80,
                "prediction_distance_metres": 0.002,
                "restitution": 0.20,
                "restitution_minimum_speed_metres_per_second": 1,
                "velocity_tolerance_metres_per_second": 1e-05
            },
            "integration_owner": "rust_world_candidate",
            "maximum_initial_overlap_metres": 0.10,
            "physical_frequency_hertz": 500,
            "road_query_extension_metres": 0.15,
            "schema_version": 1,
            "steering_rack_travel_metres": 0.04,
            "suspension": {
                "differentiation_step_metres": 0.0001,
                "gravity_world_metres_per_second_squared": {"x": 0, "y": -9.80665, "z": 0},
                "maximum_substep_seconds": 0.002,
                "tire_vertical_damping_newton_seconds_per_metre": [1100, 1100, 1100, 1100],
                "tire_vertical_stiffness_newtons_per_metre": [180000, 180000, 180000, 180000],
                "travel_stop_damping_newton_seconds_per_metre": [800, 800, 800, 800],
                "travel_stop_engagement_fraction": [0.85, 0.85, 0.85, 0.85],
                "travel_stop_stiffness_newtons_per_cubic_metre": [80000000, 80000000, 80000000, 80000000]
            }
        }
    }

    # 12. driving_assistance.json
    driving_assistance = {
        "aids": {
            "abs_available": False,
            "abs_default_enabled": False,
            "abs_player_selectable": False,
            "auto_clutch_default_enabled": True,
            "auto_clutch_player_selectable": True,
            "brake_assist_available": False,
            "brake_assist_default_enabled": False,
            "brake_assist_force_multiplier": 1.0,
            "brake_assist_player_selectable": False,
            "countersteer_default_enabled": False,
            "handbrake_abs_interlock": False,
            "handbrake_clutch_coupling": True,
            "handbrake_rear_only": True,
            "input_smoothing_brake_rate": 16.0,
            "input_smoothing_enabled": True,
            "input_smoothing_steering_rate": 8.0,
            "input_smoothing_throttle_rate": 6.0,
            "launch_control_available": False,
            "launch_control_clutch_release_rate_per_s": 2.0,
            "launch_control_default_enabled": False,
            "launch_control_player_selectable": False,
            "launch_control_target_rpm": 0.0,
            "launch_control_throttle_limit": 1.0,
            "stability_available": False,
            "stability_default_enabled": False,
            "stability_grounded_multiplier": 1.0,
            "stability_player_selectable": False,
            "stability_reference_lateral_accel_m_s2": 15.0,
            "stability_upright_damping": 0.0,
            "stability_upright_spring": 0.0,
            "stability_yaw_engage_angle_rad": 0.1,
            "stability_yaw_strength": 0.0,
            "steering_assist_player_selectable": False,
            "steering_slip_assist_default_enabled": False,
            "traction_control_actuator": "engine_torque",
            "traction_control_attack_rate": 30.0,
            "traction_control_available": False,
            "traction_control_cut_gain": 1.0,
            "traction_control_default_enabled": False,
            "traction_control_gear_authority": [0.0, 0.0, 0.0, 0.0, 0.0],
            "traction_control_gear_max_cut": [0.0, 0.0, 0.0, 0.0, 0.0],
            "traction_control_gear_slip_target": [0.0, 0.0, 0.0, 0.0, 0.0],
            "traction_control_player_selectable": False,
            "traction_control_release_rate": 15.0,
            "traction_control_slip_threshold": 0.15
        }
    }

    # 13. suspension_filters.json
    suspension_filters = {
        "suspension_filters": {
            "road_height_filter_tau_s": 0.0,
            "road_height_max_rate_m_s": 20.0,
            "front": {
                "arb_travel_filter_tau_s": 0.0,
                "damper_velocity_filter_tau_s": 0.0
            },
            "rear": {
                "arb_travel_filter_tau_s": 0.0,
                "damper_velocity_filter_tau_s": 0.0
            }
        }
    }

    # 14. tire_filters.json
    tire_filters = {
        "tire_filters": {
            "surface_transition_tau_s": 0.08,
            "front": {
                "contact_ramp_tau_s": 0.05,
                "slip_loss_tau_s": 0.06,
                "slip_recovery_tau_s": 0.20,
                "min_relaxation_tau_s": 0.010,
                "relaxation_recovery_scale": 1.2,
                "combined_lateral_peak_migration": 0.30,
                "combined_longitudinal_peak_migration": 0.25
            },
            "rear": {
                "contact_ramp_tau_s": 0.05,
                "slip_loss_tau_s": 0.06,
                "slip_recovery_tau_s": 0.20,
                "min_relaxation_tau_s": 0.010,
                "relaxation_recovery_scale": 1.2,
                "combined_lateral_peak_migration": 0.30,
                "combined_longitudinal_peak_migration": 0.25
            }
        }
    }

    files = {
        "vehicle.json": manifest,
        "chassis.json": chassis,
        "powertrain.json": powertrain,
        "suspension.json": suspension,
        "tires.json": tires,
        "brakes.json": brakes,
        "aerodynamics.json": aerodynamics,
        "steering.json": steering,
        "fuel.json": fuel,
        "underfloor_contact.json": underfloor_contact,
        "coupled_world.json": coupled_world,
        "driving_assistance.json": driving_assistance,
        "suspension_filters.json": suspension_filters,
        "tire_filters.json": tire_filters
    }

    from calibrate_ford_rally_profile import calibrate
    provenance = calibrate(files)
    files['port_validation.json'] = provenance

    for out_dir in output_dirs:
        os.makedirs(out_dir, exist_ok=True)
        for fname, content in files.items():
            fpath = os.path.join(out_dir, fname)
            with open(fpath, "w", encoding="utf-8") as f:
                json.dump(content, f, indent=2, sort_keys=True)
            print(f"Escrito: {fpath}")

if __name__ == "__main__":
    dirs = [
        r"D:\open-formula\distribution\share\games\speed-dreams-2\cars\rust\ford-escort-rs2000",
        r"D:\open-formula\rust\data\vehicles\ford-escort-rs2000"
    ]
    build_ford_escort_profile(dirs)

