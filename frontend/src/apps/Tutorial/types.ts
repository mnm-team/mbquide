export type TutorialMedia =
  | { type: 'video'; src: string }
  | { type: 'image'; src: string };

// An optional labeled block of further reading on a step's detail page - e.g. "Mathematical
// foundation", "Related operations", "Keyboard shortcut". Purely additive: a step with no
// `sections` still gets a detail page, just with only the video + description on it.
export type TutorialSection = {
  heading: string;
  paragraphs: string[];
};

export type TutorialStep = {
  id: string;
  page: string;
  title: string;
  description: string;
  media: TutorialMedia;
  sections?: TutorialSection[];
};
