from pathlib import Path
import time, statistics, json
import verify_telemetry as m
# Use the same ABI bindings and real Melbourne mesh as the verifier.
s=Path(m.__file__).read_text(encoding="utf-8")
exec("if True:\n"+s[s.index("    OUT.mkdir"):s.index("    origin =")],m.__dict__)
out=Path(__file__).resolve().parent/"async_benchmark";out.mkdir(exist_ok=True)
m.dll.ofp_trace_configure(str(out).encode())
h=m.dll.ofp_create(str(m.PROFILE).encode(),m.V(-1007.681,51.871,-829.024),-.005,0.)
steps=[]
try:
    for _ in range(800):
        t=time.perf_counter(); code=m.dll.ofp_step(h,m.C.byref(m.I(clutch=1.,gear_request=0)),None,0,.002)
        steps.append((time.perf_counter()-t)*1000)
        if code: raise RuntimeError("benchmark step rejected")
finally:
    m.dll.ofp_destroy(h)
m.dll.ofp_trace_flush();m.dll.ofp_trace_shutdown()
windows=[json.loads(p.read_text()) for p in out.glob("rustphysics_*.json")]
r={"steps":len(steps),"median_step_ms":statistics.median(steps),"p99_step_ms":sorted(steps)[int(.99*len(steps))],"max_step_ms":max(steps),
   "windows":[{k:w[k] for k in ["rows","writer_thread","capture_ms","format_write_ms","dropped_windows"]} for w in windows]}
(out/"metrics.json").write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))
