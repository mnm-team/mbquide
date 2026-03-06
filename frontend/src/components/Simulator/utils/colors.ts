import { NodeType } from '../types';
import { getFillColor } from '../../Graph/utils/colors'

export const getFillColorForSimulator = (
  node: NodeType,
  measured: number[],
  active: number[],
): string => {

  // Override color for measured nodes
  if (measured.includes(node.id)) {
    return "grey";
  }

  // Override color for non-active nodes
  if (!active.includes(node.id)) {
    // Get the original color and modify it to have transparency
    const originalColor = getFillColor(node);
    console.log("Not active: " + node.id);
    return setTransparency(originalColor, 0.6);
  }
  
  return getFillColor(node);

};



// Function to set transparency on a color string
const setTransparency = (color: string, alpha: number): string => {
  // If the color is a hex code, convert it to RGBA
  if (color.startsWith('#')) {
    const rgba = hexToRgba(color, alpha);
    return rgba;
  }
  // If the color is already in a format that can accept alpha (e.g., RGB), just append the alpha
  if (color.startsWith('rgb')) {
    return color.replace('rgb', 'rgba').replace(')', `, ${alpha})`);
  }
  return color;  // Return unchanged if not in expected format
};

// Function to convert hex color to RGBA
const hexToRgba = (hex: string, alpha: number): string => {
  let r = 0, g = 0, b = 0;

  // 3 digits
  if (hex.length === 4) {
    r = parseInt(hex[1] + hex[1], 16);
    g = parseInt(hex[2] + hex[2], 16);
    b = parseInt(hex[3] + hex[3], 16);
  }
  // 6 digits
  else if (hex.length === 7) {
    r = parseInt(hex[1] + hex[2], 16);
    g = parseInt(hex[3] + hex[4], 16);
    b = parseInt(hex[5] + hex[6], 16);
  }
  
  return `rgba(${r}, ${g}, ${b}, ${alpha})`;
};