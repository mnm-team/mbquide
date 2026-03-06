import { Dispatch, SetStateAction } from "react";

type BuildingModeToggleProps = {
  buildingMode: boolean;
  setBuildingMode: Dispatch<SetStateAction<boolean>>;
};

export const BuildingModeToggle = ({ buildingMode, setBuildingMode }: BuildingModeToggleProps) => {
  return (
    <div className="absolute top-8 right-8 z-10">
      <label className="relative inline-flex cursor-pointer items-center">
        <span className="ml-0 mr-3 text-lg font-medium text-black">
          Building Mode
        </span>

        <input
          type="checkbox"
          className="peer sr-only"
          checked={buildingMode}
          onChange={() => setBuildingMode(!buildingMode)}
        />

      <div
        className="
          relative h-6 w-11 rounded-full bg-gray-600
          transition-colors
          peer-checked:bg-green-500

          after:absolute after:left-[2px] after:top-[2px]
          after:h-5 after:w-5 after:rounded-full
          after:border after:border-gray-300 after:bg-white
          after:transition-all

          after:content-[url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiIgdmlld0JveD0iMCAwIDI0IDI0IiBmaWxsPSJub25lIiBzdHJva2U9IiNENEQ0RDgiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIj48cGF0aCBkPSJNMTggNiA2IDE4Ii8+PHBhdGggZD0ibTYgNiAxMiAxMiIvPjwvc3ZnPg==')]

          peer-checked:after:translate-x-full
          peer-checked:after:border-white
          peer-checked:after:content-[url('data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNiIgaGVpZ2h0PSIxNiIgdmlld0JveD0iMCAwIDI0IDI0IiBmaWxsPSJub25lIiBzdHJva2U9IiMyMkM1NUUiIHN0cm9rZS13aWR0aD0iMiIgc3Ryb2tlLWxpbmVjYXA9InJvdW5kIiBzdHJva2UtbGluZWpvaW49InJvdW5kIj48cG9seWxpbmUgcG9pbnRzPSIyMCA2IDkgMTcgNCAxMiIvPjwvc3ZnPg==')]
        "
      />
    </label>
  </div>
   );
};
