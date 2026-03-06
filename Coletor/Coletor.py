import os
import requests
import time
import logging
import smtplib
import pandas as pd
from datetime import datetime
from pathlib import Path
from requests.exceptions import Timeout, ConnectionError, HTTPError
from email.message import EmailMessage
from dotenv import load_dotenv

load_dotenv()

# Tempo do início da execução
timestamp_inicio = datetime.now().strftime('%Y-%m-%d_%H:%M:%S')

# Configurações da captura
URL = f"http://{os.getenv("URL")}/"
EMAIL_USER = os.getenv("EMAIL_USER")
EMAIL_PASS = os.getenv("EMAIL_PASS")
EMAILS = os.getenv("EMAILS", "").split(",")

TOTAL_LEITURAS = int(os.getenv("TOTAL_LEITURAS"))
INTERVALO = int(os.getenv("INTERVALO"))
TENTATIVAS = int(os.getenv("TENTATIVAS"))
ACERTO = False

BASE_DIR = Path(os.getenv("BASE_DIR")) / "Leituras"
BASE_DIR.mkdir(exist_ok=True)

logging.basicConfig(
    level = logging.WARNING,
    format = "%(asctime)s - %(levelname)s - %(message)s"
)

data = []

# Função para envio do email de alerta
def enviar_email(mensagem_erro):
    mensagem = EmailMessage()
    mensagem["Subject"] = "ALERTA - Falha repetida na coleta dos sensores"
    mensagem["From"] = EMAIL_USER
    mensagem["To"] = ", ".join(EMAILS)

    mensagem.set_content(f"""Falha repetida encontrada.\n\nErro: {mensagem_erro}\n\nHorário: {datetime.now().strftime('%d-%m-%Y %H:%M:%S')}""")
    
    with smtplib.SMTP_SSL("smtp.gmail.com", 465) as smtp:
        smtp.login(EMAIL_USER, EMAIL_PASS)
        smtp.send_message(mensagem)

# Captura da leitura pelo JSON
for i in range(TOTAL_LEITURAS):
    ultimo_erro = None

    for j in range(TENTATIVAS):
        try:
            response = requests.get(URL, timeout=5)
            response.raise_for_status()
            json_data = response.json()

            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            for sensor in json_data["sensors"]:
                    data.append({
                        "timestamp": timestamp,
                        "sensor_id": sensor["id"],
                        "sensor_type": sensor["type"],
                        "value": sensor["value"],
                        "unit": sensor["unit"]
                    })

            print(f"[OK] Coleta {i+1}/{TOTAL_LEITURAS}")

            ACERTO = True

        except Timeout:
            ultimo_erro = f"Timeout ao acessar {URL}"
            logging.warning(f"Leitura {i+1}: Timeout ao acessar {URL}")

        except ConnectionError:
            ultimo_erro = f"Falha de conexão com {URL}"
            logging.error(f"Leitura {i+1}:Falha de conexão com {URL}")

        except HTTPError as httperr:
            ultimo_erro = f"Erro HTTP em {URL}: {httperr.response.status_code}"
            logging.error(f"Letura {i+1}: Erro HTTP {httperr.response.status_code}")

        except ValueError:
            ultimo_erro = f"Json inválido recebido de {URL}"
            logging.error(f"Leitura {i+1}: Json inválido recebido")

        except Exception:
            ultimo_erro = "Erro inesperado(bug)"
            logging.exception(f"Leitura {i+1}: Erro inesperado(bug)")

        finally:
            if(ACERTO):
                ACERTO = False
                break
            
            if(j + 1 == TENTATIVAS):
                enviar_email(ultimo_erro)

            time.sleep(5)

    time.sleep(INTERVALO)

df = pd.DataFrame(data)

# Componentes de cada coluna do arquivo
df["column"] = (
    "sensor_id " + df["sensor_id"].astype(str) + 
    " sensor_type " + df["sensor_type"] +
    " unit " + df["unit"]
)

# Tabela pivot com timestamp como index e sensores como coluna
df_pivot = df.pivot_table(
    index = "timestamp",
    columns = "column",
    values = "value",
    aggfunc = "first"
)

# Ordena as colunas pelo numero do sensor_id
df_pivot = df_pivot.reindex(
    sorted(df_pivot.columns, key=lambda x: int(x.split()[1])), 
    axis=1
)

# Reseta o index para virar uma coluna
df_pivot = df_pivot.reset_index()

# Salva a captura em um arquivo .csv
filename = BASE_DIR / f"leitura_{timestamp_inicio}.csv"
df_pivot.to_csv(filename, index=False)