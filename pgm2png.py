import argparse
import sys
from pathlib import Path

from PIL import Image


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convierte frames PGM a PNG y opcionalmente crea un GIF."
    )
    parser.add_argument("input_dir", help="carpeta con los archivos .pgm")
    parser.add_argument("output_dir", help="carpeta para los archivos .png")
    parser.add_argument(
        "--gif",
        nargs="?",
        const="animation.gif",
        default=None,
        help="crea un GIF animado en la ruta indicada (animation.gif por defecto)",
    )
    parser.add_argument(
        "--fps",
        type=float,
        default=30.0,
        help="cuadros por segundo del GIF (30 por defecto)",
    )
    return parser.parse_args()


def load_frames(input_dir: Path) -> list[Image.Image]:
    paths = sorted(input_dir.glob("*.pgm"))
    if not paths:
        raise SystemExit(f"No se encontraron archivos .pgm en {input_dir}")

    frames = []
    for path in paths:
        with Image.open(path) as image:
            frames.append(image.convert("L"))
    return frames


def save_pngs(frames: list[Image.Image], input_dir: Path, output_dir: Path) -> None:
    paths = sorted(input_dir.glob("*.pgm"))
    total = len(paths)
    output_dir.mkdir(parents=True, exist_ok=True)
    for index, (path, frame) in enumerate(zip(paths, frames), start=1):
        frame.save(output_dir / f"{path.stem}.png")
        print(f"\rConvirtiendo {index}/{total}", end="", flush=True)
    print()


def save_gif(frames: list[Image.Image], gif_path: Path, fps: float) -> None:
    size = frames[0].size
    if any(frame.size != size for frame in frames):
        raise SystemExit("Los frames no tienen el mismo tamano; no se puede crear el GIF")
    if fps <= 0.0:
        raise SystemExit("Los FPS deben ser mayores que cero")

    duration = max(10, round(1000.0 / fps))
    gif_path.parent.mkdir(parents=True, exist_ok=True)
    palette = [frame.convert("P", palette=Image.Palette.ADAPTIVE) for frame in frames]
    palette[0].save(
        gif_path,
        save_all=True,
        append_images=palette[1:],
        duration=duration,
        loop=0,
        optimize=True,
    )


def main() -> int:
    args = parse_args()
    input_dir = Path(args.input_dir)
    output_dir = Path(args.output_dir)

    if not input_dir.is_dir():
        print(f"No existe la carpeta de entrada: {input_dir}", file=sys.stderr)
        return 1

    frames = load_frames(input_dir)
    save_pngs(frames, input_dir, output_dir)
    print(f"PNG generados: {len(frames)} en {output_dir}")

    if args.gif is not None:
        gif_path = Path(args.gif)
        save_gif(frames, gif_path, args.fps)
        print(f"GIF generado: {gif_path} ({len(frames)} frames, {args.fps:g} fps)")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
