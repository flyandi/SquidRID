
/**
  _____  ___   __ __  ____  ___    ____   ____  ___
 / ___/ /   \ |  |  ||    ||   \  |    \ |    ||   \
(   \_ |     ||  |  | |  | |    \ |  D  ) |  | |    \
 \__  ||  Q  ||  |  | |  | |  D  ||    /  |  | |  D  |
 /  \ ||     ||  :  | |  | |     ||    \  |  | |     |
 \    ||     ||     | |  | |     ||  .  \ |  | |     |
  \___| \__,_| \__,_||____||_____||__|\_||____||_____|

 *
 * This file is part of SquidRID (https://github.com/flyandi/squidrid)
 *
 * Copyright (c) 2023 FLY&I (flyandi.net)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/
import { useEffect, useState } from "react";
import './App.css';
import 'leaflet/dist/leaflet.css';
import { MapContainer, TileLayer, useMapEvents, Circle, Polyline, FeatureGroup, Popup } from 'react-leaflet'
import { BrowserSerial } from "browser-serial";
import Control from "react-leaflet-custom-control";
import { Dropdown, toCombinedPath, fromPath, toPath, inflatePath, deflatePath } from "./common";
import { idtype_t, uatype_t } from "./squid";


const serial = new BrowserSerial();

const Commands = {
  version: "V",
  data: "D",
  reboot: "R",
  current: "C",
  store_data: "SD",
  store_mode: "SM",
  path: "P",
}

const MapMode = {
  Pan: 0,
  Path: 1,
  Origin: 2,
  Operator: 3,
}

const MapPestMode = {
  Pan: 0,
  Origin: 4,
}

const AppMode = {
  Sim: 0,
  Pest: 1,
  //Real: 3, // future
}

const FlyMode = {
  Pause: 0,
  Fly: 1,
}

const PathMode = {
  Hold: 0,
  Random: 1,
  Follow: 2,
}

const RadiusMode = [500, 1000, 2000, 3000, 5000, 10000];
const MinRadius = 500;


const Button = ({ name, secondary, selected, onPress }) => {
  return (
    <button className={[selected ? "selected" : "", secondary ? "secondary" : ""].join(" ")} onClick={onPress}>{name}</button>
  )
};

const Display = ({ condition, children }) => {
  if (!condition) return;
  return children;
}

const RemoteControlButton = ({ name, selected = false, steps, onChange }) => {
  const handle = value => () => onChange(value);
  return (
    <div className="remote-control">
      <button className="text" disabled>{name}</button>
      {(steps || []).map(step =>
        <Button selected={selected === step} key={step} name={step} onPress={handle(step)} />
      )}
    </div>
  )
}

const F = (n, p = 6) => {
  const roundedNumber = parseFloat(n || 0).toFixed(p);
  const paddedNumber = roundedNumber.padEnd(roundedNumber.indexOf('.') + p + 1, '0');
  return Number(paddedNumber);
};
const I = (n) => Number(parseInt(n) || 0);
const S = (s) => ((s || s === 0) && s.length ? s : " ").toUpperCase();


const LocationMarker = ({ onEvent }) => {
  useMapEvents({
    click(e) {
      onEvent(e);
    },
    locationfound(e) {
      onEvent(e);
    },
  })
  return null;
}



function App() {

  const [connected, setConnected] = useState(false);
  const [position, setPosition] = useState({ lat: 0, lng: 0, op_lat: 0, op_lng: 0, alt: 0, spd: 0 });
  const [pest, setPest] = useState({});
  const [data, setData] = useState({ pe_lat: 0, pe_lng: 0, pe_radius: MinRadius, pe_spawn: 5 });
  const [dataUpdated, setDataUpdated] = useState(false);
  const [map, setMap] = useState(null)
  const [mapMode, setMapMode] = useState(MapMode.Pan);
  const [flyMode, setFlyMode] = useState(FlyMode.Idle);
  const [pathMode, setPathMode] = useState(PathMode.Idle);
  const [zoom] = useState(16);
  const [lock, setLock] = useState(false);
  const [appMode, setAppMode] = useState(AppMode.Sim);
  const [trail, setTrail] = useState([]);
  const [path, setPath] = useState([]);
  const [showProfile, setShowProfile] = useState(false);
  const [showPath, setShowPath] = useState(false);
  const [error, setError] = useState(false);
  const [first, setFirst] = useState(true);

  const serialCommand = (command, args = []) => {
    if (connected) {
      const cmd = ["$", command].join("") + (args.length ? "|" + args.join("|") : "");
      console.log("[serial]", "Writing", cmd);
      serial.write(cmd);
    }
  }

  const handleSync = (o = true) => {
    if (o && dataUpdated) {
      handleDataUpdate();
    } else {
      serialCommand(Commands.data);
      serialCommand(Commands.current);
    }
  }

  const handleSerialConnect = () => {
    setError(false);
    if (connected) {
      serial.disconnect().then(() => setConnected(false)).catch(e => {
        console.log(e);
        setError(e.toString());
        setConnected(false);
      });
    } else {
      serial.connect().then(() => {
        setConnected(true);
      }).catch(e => {
        console.log(e);
        setError(e.toString());
        setConnected(false);
      });
    }
  }

  const handleData = name => event => {
    setDataUpdated(true);
    setData({ ...data, [name]: event.target.value });
  }

  const handleDataUpdate = (o = {}) => {
    const dt = { ...data, ...o }
    serialCommand(Commands.store_data, [S(dt.rid), S(dt.operator), S(dt.description), dt.uatype, dt.idtype, dt.lat, dt.lng, dt.alt, dt.op_lat, dt.op_lng, dt.op_alt, dt.spd, dt.sats, dt.mac, appMode, dt.pe_lat, dt.pe_lng, dt.pe_radius, dt.pe_spawn]);
  }

  const handleModeUpdate = (o = {}) => {
    const dt = { appMode, flyMode, pathMode, spd: position.spd, alt: position.alt, ...o }
    const pp = path.length ? toPath([[position.lat, position.lng], ...path, [position.lat, position.lng]]) : [];
    console.log(pp);
    serialCommand(Commands.store_mode, [dt.appMode + "", dt.flyMode + "", dt.pathMode + "", dt.spd + "", dt.alt + "", path.length, ...deflatePath(pp)]);
  }

  const handleRM = n => v => {
    handleModeUpdate({ [n]: v });
  }

  const handleAppMode = appMode => () => {
    setAppMode(appMode);
    handleModeUpdate({ appMode });
  }

  const handleFlyMode = flyMode => () => {
    setFlyMode(flyMode);
    handleModeUpdate({ flyMode });
  }

  const handlePathMode = pathMode => () => {
    setPathMode(pathMode);
    handleModeUpdate({ pathMode });
  }

  const handleMapCenter = () => {
    map.setView([parseFloat(position.lat || 0), parseFloat(position.lng || 0)], zoom);
  }

  const handleMapOPCenter = () => {
    map.setView([parseFloat(position.op_lat || 0), parseFloat(position.op_lng || 0)], zoom);
  }

  const handlePEMapCenter = () => {
    map.setView([parseFloat(data.pe_lat || 0), parseFloat(data.pe_lng || 0)], zoom);
  }

  const handleMapLocationCenter = () => {
    map.locate();
  }

  const handleShowProfile = d => () => {
    setShowProfile(d);
  }

  const handleShowPath = d => () => {
    setShowPath(d);
  }
  const handlePestRadius = v => {
    const dt = { pe_radius: RadiusMode[v - 1] || MinRadius };
    setData({ ...data, ...dt })
    handleDataUpdate(dt);
  }

  const handlePestSpawn = v => {
    const dt = { pe_spawn: v || 5 };
    setData({ ...data, ...dt })
    handleDataUpdate(dt);
  }

  const handleClearPath = () => {
    setPath([...[]]);
  }

  const handleSyncPath= () => {
    handleShowPath(false)();
    handleModeUpdate();
  }

  const handleSerialValue = (value) => {
    if (!value) {
      return;
    }
    if (value.substr(0, 1) === "$") {
      const c = value.substr(1, 1);
      const p = value.split("|");
      console.log("[serial] received", value, " / ", p);
      if (c === "%") {
        setLock(false);
        setDataUpdated(false);
        handleSync(false);
      }
      if (c === "V") {
        setData({ ...data, version: p[1] });
      }
      if (c === "C") {
        setTrail(prev => ([...prev, [p[1], p[2]]]));;
        setPosition({ ...position, lat: F(p[1]), lng: F(p[2]), op_lat: F(p[3]), op_lng: F(p[4]), alt: I(p[5]), op_alt: I(p[6]), spd: I(p[7]), hdg: I(p[8]), sats: I(p[9]) });
        setFlyMode(I(p[10]));
        setPathMode(I(p[11]));
        if (first) {
          //handleMapCenter(p[1], p[2]);
          setFirst(false);
        }
      }
      if (c === "T") {
        setPest(prev => {
          let mc = p[6];
          const u = { path: [], ...(pest[mc] || {}), lat: F(p[1]), lng: F(p[2]), alt: I(p[3]), spd: I(p[4]), hdg: I(p[5]), rid: p[7], operator: p[8], description: p[9], uatype: p[10], idtype: p[11] };
          u.path.push([u.lat, u.lng]);
          return { ...prev, [mc]: u };
        });
        setFlyMode(I(p[12]));
      }
      if (c === "D") {
        setData({ ...data, version: p[1], rid: p[2], operator: p[3], description: p[4], uatype: p[5], idtype: p[6], lat: p[7], lng: p[8], alt: p[9], op_lat: p[10], op_lng: p[11], op_alt: p[12], spd: p[13], sats: p[14], mac: p[15], pe_lat: p[17], pe_lng: p[18], pe_radius: I(p[19]), pe_spawn: I(p[20]) });
        setAppMode(I(p[16]) || AppMode.Sim);
        const pc = I(p[21]);
        const pp =  pc != 0 ? fromPath([p[7], p[8]], inflatePath(p.slice(22, -1))) : [];
        console.log("[path]", pc, pp);
        setPath(pp);

      }
    }
  }

  const handleMapEvent = e => {
    if (e.type === "click") {
      const lat = e.latlng.lat;
      const lng = e.latlng.lng;

      let dd = {};

      switch (mapMode) {
        case MapMode.Path:
          setPath(prev => [...prev, [F(lat), F(lng)]]);
          break;
        case MapMode.Origin:
          dd = { lat: F(lat), lng: F(lng) };
          setData({ ...data, ...dd });
          setPosition({ ...position, ...dd });
          handleDataUpdate(dd);
          break;
        case MapMode.Operator:
          dd = { op_lat: F(lat), op_lng: F(lng) };
          setData({ ...data, ...dd });
          setPosition({ ...position, ...dd });
          handleDataUpdate(dd);
          break;
        case MapPestMode.Origin:
          dd = { pe_lat: F(lat), pe_lng: F(lng) };
          setData({ ...data, ...dd });
          handleDataUpdate(dd);
          break;
        default:
          break;
      }
    }
    if (e.type === "locationfound") {
      map.setView(e.latlng, zoom)
    }
  }

  const handleMapMode = m => () => {
    setMapMode(m);
  }


  useEffect(() => {
    if (connected) {
      window.squid = {};
      Object.keys(Commands).forEach(cmd => window.squid[cmd] = () => serialCommand(Commands[cmd]));
      serial.readLoop((value, done) => handleSerialValue(value)).catch(e => {
        setConnected(false);
      });
      handleSync();
    }
  }, [connected]);

  if (!connected) {
    return (
      <div id="screen">
        <div>
          <div className="blink">For educational purposes only</div>
        </div>
        <h1>SquidRID</h1>
        <div className="secondary sl">FAA RemoteID Penetration and Test Tool</div>
        <div className="action">
          <Button onPress={handleSerialConnect} name={"Connect Device"} />
        </div>
        <div>
          Safe Use: This interface does not track, store or handle any user data through either cookies or other means.
        </div>
        <div>
          SquidRID is a dedicated firmware and configuration tool allowing you to control and test most aspects of the RemoteID protocol. The firmware is available at the GitHub repo below and runs on most ESP32 boards.
        </div>
        <div>
          
        </div>
        <div>
          <a href="https://github.com/flyandi/squidrid" target="_blank">https://github.com/flyandi/squidrid</a>
        </div>
        <div>
          Version 1.0
        </div>
        {error ? <div id="error"><small>{error}</small></div> : null}
      </div >
    )
  }

  return (
    <div id="map">
      <Display condition={showProfile === true}>
        <div id="modal">
          <div className="inner">
            <div className="controls">
              <Button disabled={lock} onPress={handleSync} secondary name={"Apply"} />
              <Button onPress={handleShowProfile(false)} name={"Cancel"} />
            </div>
            <div className="setting">
              <p className="secondary sl">Settings</p>
              <div className="form">
                <Dropdown items={idtype_t} selected={data.idtype} label="ID Type" onChange={handleData("idtype")} />
              </div>
              <div className="form">
                <label className="w">Remote ID</label>
                <input type="input" value={data.rid} onChange={handleData("rid")} size="24" maxLength="24" />
              </div>
              <div className="form">
                <Dropdown items={uatype_t} selected={data.uatype} label="UA Type" onChange={handleData("uatype")} />
              </div>
              {["operator", "description"].map(key =>
                <div className="form" key={key}>
                  <label className="w">{key}</label>
                  <input type="input" value={data[key]} size="24" maxLength="24" onChange={handleData(key)} />
                </div>
              )}
            </div>
            <hr />
            <div className="setting">
              <p className="secondary sl">Tuning</p>
              {[["Origin Lat", "lat"], ["Origin Lng", "lng"], ["Origin Alt", "alt"], ["Operator Lat", "op_lat"], ["Operator Lng", "op_lng"], ["Operator Alt", "op_alt"], ["Speed", "spd"], ["Satellites", "sats"], ["MAC Address", "mac"]].map(ref =>
                <div className="form" key={ref[1]}>
                  <label className="w">{ref[0]}</label>
                  <input type="input" value={data[ref[1]] || ""} size="24" maxLength="24" onChange={handleData(ref[1])} />
                </div>
              )}
            </div>
          </div>
        </div>
      </Display>
      <Display condition={showPath === true}>
        <div id="modal">
          <div className="inner">
            <div className="controls">
              <Button disabled={lock} onPress={handleSyncPath} secondary name={"Apply"} />
              <Button disabled={lock} onPress={handleClearPath} name={"Clear"} />
              <Button onPress={handleShowPath(false)} name={"Cancel"} />
            </div>
            <div className="setting">
              <p className="secondary sl">Path</p>
              {toCombinedPath(path.length ? [[position.lat, position.lng], ...path, [position.lat, position.lng]] : []).map((item, index) => {
                return (
                  <div className="pathitem">
                    <div>{index + 1}</div>
                    <div><span>LAT</span>{F(item.lat, 3)}</div>
                    <div><span>LNG</span>{F(item.lng, 3)}</div>
                    <div><span>HDG</span>{F(item.path[0])}</div>
                    <div><span>DST</span>{F(item.path[1])}</div>
                  </div>
                );
              })}
            </div>
          </div>
        </div>
      </Display>
      <MapContainer center={[position.lat, position.lng]} zoom={zoom} scrollWheelZoom={true} zoomControl={false} ref={setMap}>
        <TileLayer
          //attribution='&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors'
          //url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
          //https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}
          className="basemap"
          maxNativeZoom={17}
          maxZoom={17}
          subdomains={["clarity"]}
          url="https://stamen-tiles.a.ssl.fastly.net/toner/{z}/{x}/{y}.png"
        />
        <Display condition={appMode === AppMode.Sim}>
          <Circle color="red" center={[position.lat, position.lng]} radius={15} />
          <Circle color="blue" center={[position.op_lat, position.op_lng]} radius={10} />
          {position.lat && trail.length ? <Polyline pathOptions={{ color: "red" }} positions={[[position.lat, position.lng], ...trail]} /> : null}
          {position.lat && path.length ? <Polyline pathOptions={{ color: "green" }} positions={[[position.lat, position.lng], ...path, [position.lat, position.lng]]} /> : null}
        </Display>
        <Display condition={appMode === AppMode.Pest}>
          <Circle color="yellow" center={[data.pe_lat, data.pe_lng]} radius={data.pe_radius < 10 ? 10 : data.pe_radius} />
          {Object.keys(pest).map(mac => {
            let u = pest[mac];
            return (
              <FeatureGroup key={mac}>
                <Popup>
                  <sub>RemoteID</sub>
                  <div>{u.rid}</div>
                  <sub>Operator</sub>
                  <div>{u.operator}</div>
                  <div>{u.description}</div>
                  <sub>Speed / Alt / Heading / Lat / Lng</sub>
                  <div><span>{u.spd}</span><span>{u.alt}</span><span>{u.hdg}</span><span>{F(u.lat, 3)}</span><span>{F(u.lng, 3)}</span></div>
                </Popup>
                <Circle color="red" center={[u.lat, u.lng]} radius={15} />
                <Polyline pathOptions={{ color: "red" }} positions={[...u.path]} />
              </FeatureGroup >
            )
          })}
        </Display>

        <LocationMarker onEvent={handleMapEvent} />
        <Control position="bottomleft">
          <Display condition={mapMode === MapMode.Pan && appMode === AppMode.Sim}>
            <div className="telemetry">
              <div><span>Lat</span>{F(position.lat)}</div>
              <div><span>Lng</span>{F(position.lng)}</div>
              <div><span>Alt</span>{F(position.alt, 1)}</div>
              <div><span>Speed</span>{F(position.spd, 0)}</div>
              <div><span>Heading</span>{F(position.hdg, 0)}</div>
              <div><span>Sats</span>{F(position.sats, 0)}</div>
              <div><span>OP Lat</span>{F(position.op_lat)}</div>
              <div><span>OP Lng</span>{F(position.op_lng)}</div>
              <div><span>OP Alt</span>{F(position.op_alt, 1)}</div>
            </div>
          </Display>
        </Control>
        <Control position="bottomright" prepend>
          <div className="map-control right">
            <Display condition={appMode === AppMode.Sim}>
              {Object.keys(MapMode).map(key =>
                <Button key={key} disabled={lock} selected={mapMode === MapMode[key]} name={key} onPress={handleMapMode(MapMode[key])} />
              )}
            </Display>
            <Display condition={appMode === AppMode.Pest}>
              {Object.keys(MapPestMode).map(key =>
                <Button key={key} disabled={lock} selected={mapMode === MapPestMode[key]} name={key} onPress={handleMapMode(MapPestMode[key])} />
              )}
            </Display>

          </div>
        </Control>
        <Control position="bottomleft">
          <Display condition={appMode === AppMode.Sim}>
            <div className="map-control sl">
              <div>{data.operator || "No Name"}</div>
              <div>{data.rid || "No ID"}</div>
            </div>
          </Display>
          <div className="map-control inline">
            <Display condition={appMode === AppMode.Sim}>
              <Button name="Center" onPress={handleMapCenter} />
              <Button name="OP" onPress={handleMapOPCenter} />
            </Display>
            <Display condition={appMode === AppMode.Pest}>
              <Button name="Center" onPress={handlePEMapCenter} />
            </Display>
            <Button name="Loc" onPress={handleMapLocationCenter} />
          </div>
        </Control>
        <Control position="topright" prepend>
          <div className="map-control right">
            <Button onPress={handleSerialConnect} name={connected ? "Disconnect" : "Connect"} />
            <Button disabled={lock} onPress={handleSync} name={"Sync"} />
            <Button name="Profile" onPress={handleShowProfile(true)} />
            <Button name="Path" onPress={handleShowPath(true)} />
          </div>
        </Control>
        <Control position="topleft" prepend>
          <div className="map-control inline">
            {Object.keys(AppMode).map(key =>
              <Button key={key} disabled={lock} selected={appMode === AppMode[key]} name={key} onPress={handleAppMode(AppMode[key])} />
            )}
          </div>
          <div className="map-control inline">
            {Object.keys(FlyMode).map(key =>
              <Button key={key} disabled={lock} selected={flyMode === FlyMode[key]} name={key} onPress={handleFlyMode(FlyMode[key])} />
            )}
          </div>
          <Display condition={mapMode === MapMode.Pan && appMode === AppMode.Sim}>
            <div className="map-control inline">
              {Object.keys(PathMode).map(key =>
                <Button key={key} disabled={lock} selected={pathMode === PathMode[key]} name={key} onPress={handlePathMode(PathMode[key])} />
              )}
            </div>
          </Display>
          <Display condition={mapMode === MapMode.Pan && appMode === AppMode.Sim}>
            <div className="map-control inline">
              <RemoteControlButton name="SPD" steps={[0, 50, 100, 200]} onChange={handleRM("spd")} />
              <RemoteControlButton name="ALT" steps={[0, 100, 500, 1000]} onChange={handleRM("alt")} />
            </div>
          </Display>
          <Display condition={appMode === AppMode.Pest}>
            <div className="map-control inline">
              <RemoteControlButton name="SPW" selected={data.pe_spawn} steps={[5, 10, 20, 30, 45]} onChange={handlePestSpawn} />
              <RemoteControlButton name="RNG" steps={[1, 2, 3, 4, 5, 6]} onChange={handlePestRadius} />
              <p>RANGE {data.pe_radius || MinRadius}m EVERY {data.pe_spawn}s</p>
            </div>
          </Display>
        </Control>
      </MapContainer >
    </div >
  );
}

export default App;



