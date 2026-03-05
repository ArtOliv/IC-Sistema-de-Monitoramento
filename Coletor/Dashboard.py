import re
import os
import streamlit as st
import pandas as pd
import plotly.express as px
from pathlib import Path
from dotenv import load_dotenv

load_dotenv()

st.set_page_config(layout="wide")
st.title("Dashboard de Monitoramento de Sensores")

BASE_DIR = Path(os.getenv("BASE_DIR")) / "Leituras"
BASE_DIR.mkdir(exist_ok=True)

# Lê arquivos de leitura, concatena, e organiza eles
@st.cache_data
def carregar_dados(limite_arquivos, ultima_modificacao):
    arquivos = sorted(
        BASE_DIR.glob("*.csv"),
        key=lambda x: x.stat().st_mtime,
        reverse=True
    )[:limite_arquivos]

    if(not arquivos):
        return pd.DataFrame()
    
    dfs = [pd.read_csv(arq) for arq in arquivos]
    df = pd.concat(dfs, ignore_index=True)

    df["timestamp"] = pd.to_datetime(df["timestamp"])
    df = df.sort_values("timestamp").reset_index(drop=True)

    return df

arquivos = list(BASE_DIR.glob("*.csv"))

if(arquivos):
    ultima_modificacao = max(f.stat().st_mtime for f in arquivos)
else:
    ultima_modificacao = 0

df = carregar_dados(400, ultima_modificacao)

if(df.empty):
    st.warning("Nenhum arquivo encontrado na pasta Leituras.")
    st.stop()

# Extrai dados dos sensores presentes no nome das colunas
sensor_cols = [col for col in df.columns if col != "timestamp"]

for col in sensor_cols:
    df[col] = pd.to_numeric(df[col], errors="coerce") # Converte os dados da leitura para numérico

sensor_info = []

for col in sensor_cols:
    match = re.search(r"sensor_id\s+(\d+)\s+sensor_type\s+(\w+)\s+unit\s+(.+)", col)

    if(match):
        sensor_id = int(match.group(1))
        sensor_type = match.group(2)
        unit = match.group(3)

        sensor_info.append({
            "coluna": col,
            "id": sensor_id,
            "tipo": sensor_type,
            "unidade": unit
        })

meta_df = pd.DataFrame(sensor_info)

# Filtros dinâmicos para exibição no gráfico
st.sidebar.header("Filtros")

# Filtro por período de tempo
data_min = df["timestamp"].min()
data_max = df["timestamp"].max()

periodo = st.sidebar.date_input("Selecione o período de leituras", [data_min, data_max])

if(len(periodo) == 2):
    inicio = pd.to_datetime(periodo[0])
    fim = pd.to_datetime(periodo[1]) + pd.Timedelta(days=1)

    df = df[(df["timestamp"] >= inicio) & (df["timestamp"] < fim)]

# Filtro por tipo de sensor
tipos = st.sidebar.multiselect(
    "Tipo de sensor",
    sorted(meta_df["tipo"].unique())
)

colunas_filtradas = meta_df.copy()

if(tipos):
    colunas_filtradas = colunas_filtradas[colunas_filtradas["tipo"].isin(tipos)]

colunas_plot = colunas_filtradas["coluna"].tolist()

# Prapara dados e plota o gráfico na interface
df_long = df.melt(
    id_vars="timestamp",
    value_vars=colunas_plot,
    var_name="sensor",
    value_name="valor"
)

df_long = df_long.sort_values(["sensor", "timestamp"])

if(df_long.empty):
    st.warning("Nenhum dado disponível para os filtros selecionados.")
else:
    fig = px.line(
        df_long,
        x="timestamp",
        y="valor",
        color="sensor",
        markers=False,
        labels={
            "timestamp": "Data/Hora",
            "valor": "Leitura",
            "sensor": "Sensor"
        }
    )

    fig.update_layout(
        height=600,
        template="plotly_dark",
        legend_title="Sensores",
        hovermode="x unified"
    )

    fig.update_traces(
        marker=dict(size=6),
        line=dict(width=2)
    )

    st.plotly_chart(fig, width="stretch")

# Botão para exportar CSV com todas as leituras concatenados
st.download_button(
    "Baixar CSV filtrado",
    df[["timestamp"] + colunas_plot].to_csv(index=False),
    file_name="dados_filtrados.csv",
    mime="text/csv"
)