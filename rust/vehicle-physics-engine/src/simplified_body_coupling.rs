//! Schur complement of the body + four axial point-mass equations.
//! Corner masses move with the body transversely, even when their axial DOF is free.
//! Intrinsic corner inertia remains in the body tensor; axial translation is split
//! exactly once. All moments use the same configured reference, not a silently
//! substituted sprung centre of mass. The body/corner iteration closes axial coupling.
use crate::simulation::{corner_unsprung_mass_kg, ForceTorqueOutput, VehicleState};
use crate::types::{Vec3, WheelIndex};
use crate::vehicle_config::VehicleConfig;

fn xyz(v: Vec3) -> [f64; 3] {
    [v.x, v.y, v.z]
}
fn vec(v: &[f64]) -> Vec3 {
    Vec3::new(v[0], v[1], v[2])
}
fn columns(r: Vec3) -> [Vec3; 6] {
    [
        Vec3::RIGHT,
        Vec3::UP,
        Vec3::BACK,
        Vec3::RIGHT.cross(r),
        Vec3::UP.cross(r),
        Vec3::BACK.cross(r),
    ]
}
fn add_wrench(rhs: &mut [f64; 6], f: Vec3, r: Vec3) {
    let t = r.cross(f);
    for (i, x) in xyz(f).into_iter().chain(xyz(t)).enumerate() {
        rhs[i] += x;
    }
}

pub fn hub_arm(config: &VehicleConfig, state: &VehicleState, wheel: WheelIndex) -> Vec3 {
    let center = crate::simulation::center_of_mass_local_for_suspension_backend(
        config,
        state.suspension.backend,
    );
    let length = if wheel.is_front() {
        config.suspension.front_spring_length
    } else {
        config.suspension.rear_spring_length
    };
    let hub = config
        .tires
        .wheel_anchor_local(&config.chassis, &config.suspension, wheel)
        - Vec3::UP * (length - state.suspension.wheels[wheel as usize].suspension_compression_m);
    state.transform.basis.transform_vector(hub - center)
}

pub fn body_accelerations(
    config: &VehicleConfig,
    state: &VehicleState,
    force: ForceTorqueOutput,
) -> Result<(Vec3, Vec3), String> {
    let props =
        crate::vehicle_mass_properties::resolve_vehicle_mass_properties_for_suspension_backend(
            config,
            state.suspension.backend,
        );
    let basis = state.transform.basis;
    let axis = basis.y;
    let omega = state.angular_velocity;
    let gravity = Vec3::new(0.0, -9.80665, 0.0);
    let masses = WheelIndex::ALL.map(|w| corner_unsprung_mass_kg(config, w));
    let body_mass = props.total_mass_kilograms - masses.iter().sum::<f64>();
    if body_mass <= 0.0 {
        return Err("Nonpositive sprung mass".into());
    }
    let mut body_tensor = props
        .inertia_about_center_of_mass
        .entries_kilogram_square_metres;
    let mut first_moment = Vec3::ZERO;
    for wheel in WheelIndex::ALL {
        let m = masses[wheel as usize];
        let r = config
            .suspension
            .geometric_suspension
            .as_ref()
            .map(|g| g.corners.get(wheel).hub_center)
            .unwrap_or_else(|| config.chassis.wheel_center_local(wheel))
            - props.center_of_mass_local_metres;
        first_moment -= r * m;
        let v = xyz(r);
        for i in 0..3 {
            for j in 0..3 {
                body_tensor[i][j] -=
                    m * (if i == j { r.length_squared() } else { 0.0 } - v[i] * v[j]);
            }
        }
    }
    let first_world = basis.transform_vector(first_moment);
    let body_i = crate::suspension_mass_properties::InertiaTensor {
        entries_kilogram_square_metres: body_tensor,
    }
    .rotated(basis);
    let mut matrix = [[0.0; 6]; 6];
    let axes = [Vec3::RIGHT, Vec3::UP, Vec3::BACK];
    for i in 0..3 {
        matrix[i][i] = body_mass;
        let cross = xyz(axes[i].cross(first_world));
        for j in 0..3 {
            matrix[j][i + 3] = cross[j];
            matrix[i + 3][j] = cross[j];
            matrix[i + 3][j + 3] = body_i.entries_kilogram_square_metres[i][j];
        }
    }
    let mut rhs = [0.0; 6];
    let f = force.force_world + gravity * body_mass - omega.cross(omega.cross(first_world));
    let t = force.torque_world + first_world.cross(gravity) - omega.cross(body_i.multiply(omega));
    for (i, x) in xyz(f).into_iter().chain(xyz(t)).enumerate() {
        rhs[i] = x;
    }
    for wheel in WheelIndex::ALL {
        let m = masses[wheel as usize];
        let r = hub_arm(config, state, wheel);
        let b = columns(r);
        let p = |v: Vec3| v - axis * v.dot(axis);
        for i in 0..6 {
            for j in 0..6 {
                matrix[i][j] += m * b[i].dot(p(b[j]));
            }
        }
        let bias = omega.cross(omega.cross(r))
            + omega.cross(axis)
                * (2.0 * state.suspension.wheels[wheel as usize].unsprung_velocity_m_s);
        add_wrench(&mut rhs, p(gravity - bias) * m, r);
        // The component of a banked-road reaction perpendicular to the allowed
        // corner travel is transmitted by the rigid linkage, not by the spring.
        let corner = &state.suspension.wheels[wheel as usize];
        add_wrench(
            &mut rhs,
            p(corner.effective_normal * corner.total_normal_force),
            r,
        );
    }
    let acceleration = solve(matrix, rhs)?;
    Ok((vec(&acceleration[..3]), vec(&acceleration[3..])))
}

fn solve(mut a: [[f64; 6]; 6], mut b: [f64; 6]) -> Result<[f64; 6], String> {
    // Positive-definite physical Schur matrix, pivoted elimination, no mass epsilon.
    for i in 0..6 {
        let pivot = (i..6)
            .max_by(|x, y| a[*x][i].abs().total_cmp(&a[*y][i].abs()))
            .unwrap();
        if !a[pivot][i].is_finite() || a[pivot][i].abs() < 1e-10 {
            return Err("Singular body/corner mass matrix".into());
        }
        a.swap(i, pivot);
        b.swap(i, pivot);
        for j in i + 1..6 {
            let factor = a[j][i] / a[i][i];
            for k in i..6 {
                a[j][k] -= factor * a[i][k];
            }
            b[j] -= factor * b[i];
        }
    }
    let mut x = [0.0; 6];
    for i in (0..6).rev() {
        x[i] = (b[i] - (i + 1..6).map(|j| a[i][j] * x[j]).sum::<f64>()) / a[i][i];
    }
    if x.iter().any(|v| !v.is_finite()) {
        return Err("Nonfinite body/corner response".into());
    }
    Ok(x)
}
