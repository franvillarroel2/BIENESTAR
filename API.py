from fastapi import FastAPI
from pydantic import BaseModel
import os
from supabase import create_client
from dotenv import load_dotenv
from fastapi.middleware.cors import CORSMiddleware

load_dotenv()

SUPABASE_URL = os.getenv("SUPABASE_URL")
SUPABASE_KEY = os.getenv("SUPABASE_ANON_KEY")

supabase = create_client(SUPABASE_URL, SUPABASE_KEY)

app = FastAPI(title="BIENESTAR IoT Gateway Backend")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Cambiado para acoplarse exactamente a tus nombres de columnas en la captura
class SensorData(BaseModel):
    nodo_id: int
    temperatura: float
    humedad: float  # Antes decía 'humidity' en el esclavo, el JSON mapeará aquí
    presencia: bool
    gas: int

@app.post("/data")
def insert_data(data: SensorData):
    print(f"Recibido desde Maestro LoRa: {data}")
    try:
        registro = data.dict()
        # Nombre corregido a tu tabla real de Supabase
        res = supabase.table("Bienestar-IoT").insert(registro).execute()
        return {"status": "success", "data": res.data}
    except Exception as e:
        print("Error al insertar en Supabase:", e)
        return {"status": "error", "message": str(e)}

@app.get("/data")
def get_historical_data(limit: int = 15):
    try:
        res = supabase.table("Bienestar-IoT") \
            .select("*") \
            .limit(limit) \
            .execute()
        return list(res.data)
    except Exception as e:
        print("Error en GET /data:", e)
        return []