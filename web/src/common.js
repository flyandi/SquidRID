export const Dropdown = ({ label, items, selected, value, onChange, invert = false }) => {

    const handleChange = (v) => {
        onChange(v.nativeEvent);
    }

    return (
        <div>
            {label ? <label>{label}</label> : null}
            <select
                value={selected}
                defaultValue={selected}
                onChange={handleChange}
            >
                {Object.keys(items).map(key =>
                    <option key={key} value={invert ? key : items[key]}>{invert ? items[key] : key}</option>
                )}
            </select>
        </div>
    )
}


export const toRadians = (degrees) => degrees * (Math.PI / 180);
export const toDegrees = (radians) => radians * (180 / Math.PI);


export const deflatePath = (path) => path.flatMap(([a, b]) => [a, b]);
export const inflatePath = (flattenedPath) => {
    const path = [];
    for (let i = 0; i < flattenedPath.length; i += 2) {
        path.push([flattenedPath[i], flattenedPath[i + 1]]);
    }
    return path;
};

export const toPath = (coordinates) =>
    coordinates.slice(0, -1).map((coord, i) => {
        const [lat1, lon1] = coord;
        const [lat2, lon2] = coordinates[i + 1];

        const R = 6371e3; // Earth's radius in meters
        const φ1 = toRadians(lat1);
        const φ2 = toRadians(lat2);
        const Δφ = toRadians(lat2 - lat1);
        const Δλ = toRadians(lon2 - lon1);

        const a =
            Math.sin(Δφ / 2) * Math.sin(Δφ / 2) +
            Math.cos(φ1) * Math.cos(φ2) * Math.sin(Δλ / 2) * Math.sin(Δλ / 2);
        const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
        const distance = R * c; // Distance between coordinates in meters

        const y = Math.sin(Δλ) * Math.cos(φ2);
        const x =
            Math.cos(φ1) * Math.sin(φ2) -
            Math.sin(φ1) * Math.cos(φ2) * Math.cos(Δλ);
        const heading = (toDegrees(Math.atan2(y, x)) + 360) % 360; // Heading in degrees

        return [parseInt(heading), parseInt(distance)];
    });

export const toCombinedPath = (coordinates) =>
    toPath(coordinates).map((u, n) => ({ path: u, lat: coordinates[n][0], lng: coordinates[n][1] }));

export const fromPath = (startingCoordinate, headingsAndLengths) =>
    [startingCoordinate, ...headingsAndLengths].reduce(
        ([coordinates, [lat1, lon1]], [heading, distance]) => {
            const R = 6371e3; // Earth's radius in meters
            const δ = distance / R; // Angular distance in radians
            const φ1 = toRadians(lat1);
            const λ1 = toRadians(lon1);
            const θ = toRadians(heading);

            const lat2 = toDegrees(
                Math.asin(
                    Math.sin(φ1) * Math.cos(δ) +
                    Math.cos(φ1) * Math.sin(δ) * Math.cos(θ)
                )
            );
            const lon2 = toDegrees(
                λ1 +
                Math.atan2(
                    Math.sin(θ) * Math.sin(δ) * Math.cos(φ1),
                    Math.cos(δ) - Math.sin(φ1) * Math.sin(toRadians(lat2))
                )
            );

            return [coordinates.concat([[lat2, lon2]]), [lat2, lon2]];
        },
        [[], startingCoordinate]
    )[0];