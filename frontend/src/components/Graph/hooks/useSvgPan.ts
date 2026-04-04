import { useRef, useState, useCallback, RefObject } from 'react';

export function useSvgPan(svgRef: RefObject<SVGSVGElement | null>) {
  const [translate, setTranslate] = useState({ x: 0, y: 0 });
  const isPanning = useRef(false);
  const startPoint = useRef({ x: 0, y: 0 });
  const startTranslate = useRef({ x: 0, y: 0 });

  const getScale = useCallback(() => {
    if (!svgRef.current) return 1;
    const { width } = svgRef.current.getBoundingClientRect();
    return 1920 / width;
  }, [svgRef]);

  const startPan = useCallback((e: React.PointerEvent<SVGSVGElement>) => {
    isPanning.current = true;
    startPoint.current = { x: e.clientX, y: e.clientY };
    startTranslate.current = { ...translate };
    (e.currentTarget as SVGSVGElement).setPointerCapture(e.pointerId);
    e.currentTarget.style.cursor = 'grabbing';
  }, [translate]);

  const onPointerDown = useCallback((e: React.PointerEvent<SVGSVGElement>) => {
    const isCtlLeftClick = e.button === 0 && e.ctrlKey;
    if (!isCtlLeftClick) return;
    startPan(e);
  }, [startPan]);

  const onPointerMove = useCallback((e: React.PointerEvent<SVGSVGElement>) => {
    if (!isPanning.current) return;
    const scale = getScale();
    const dx = (e.clientX - startPoint.current.x) * scale;
    const dy = (e.clientY - startPoint.current.y) * scale;
    setTranslate({
      x: startTranslate.current.x + dx,
      y: startTranslate.current.y + dy,
    });
  }, [getScale]);

  const onPointerUp = useCallback((e: React.PointerEvent<SVGSVGElement>) => {
    if (!isPanning.current) return;
    isPanning.current = false;
  }, []);

  return { translate, onPointerDown, onPointerMove, onPointerUp };
}